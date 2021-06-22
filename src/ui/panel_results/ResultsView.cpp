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
#include "GraphViewAlpha.h"
#include "GraphViewCurve.h"

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

mMargin(5),
mOptionsW(250),
mMarginLeft(40),
mMarginRight(40),

mCurrentTypeGraph(GraphViewResults::ePostDistrib),
mCurrentVariable(GraphViewResults::eTheta),

mHasPhases(false),
mResultZoomX(1.),
mResultMinX(0.),
mResultMaxX(0.),
mResultCurrentMinX(0.),
mResultCurrentMaxX(0.),

mMajorScale(100),
mMinorCountScale(4),

mCurrentPage(0),
mGraphsPerPage(APP_SETTINGS_DEFAULT_SHEET),
mMaximunNumberOfVisibleGraph(0)
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
    mGraphTypeTabs->setFixedHeight(mGraphTypeTabs->tabHeight());

    mRuler = new Ruler(this);
    mRuler->setMarginLeft(mMarginLeft);
    mRuler->setMarginRight(mMarginRight);

    mMarker = new Marker(this);

    mEventsScrollArea = new QScrollArea(this);
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

    connect(mGraphTypeTabs, static_cast<void (Tabs::*)(const int&)>(&Tabs::tabClicked), this, &ResultsView::applyGraphTypeTab);
    connect(mRuler, &Ruler::positionChanged, this, &ResultsView::applyRuler);


    // -----------------------------------------------------------------
    //  Right Part
    // -----------------------------------------------------------------
    mOptionsScroll = new QScrollArea(this);
    mOptionsWidget = new QWidget();
    mOptionsScroll->setWidget(mOptionsWidget);
    int h = 15;

    // -----------------------------------------------------------------
    //  Results Group (if graph list tab = events or phases)
    // -----------------------------------------------------------------
    mEventsPhasesGroup = new QWidget();

    mEventsfoldCheck = new CheckBox(tr("Unfold Events"));
    mEventsfoldCheck->setFixedHeight(h);
    mEventsfoldCheck->setToolTip(tr("Display phases' events"));

    mDatesfoldCheck = new CheckBox(tr("Unfold Data"));
    mDatesfoldCheck->setFixedHeight(h);
    mDatesfoldCheck->setToolTip(tr("Display Events' data"));

    mDataThetaRadio = new RadioButton(tr("Calendar Dates"));
    mDataThetaRadio->setFixedHeight(h);
    mDataThetaRadio->setChecked(true);

    mDataSigmaRadio = new RadioButton(tr("Ind. Std. Deviations"));
    mDataSigmaRadio->setFixedHeight(h);
    
    mDataVGRadio = new RadioButton(tr("Std G"));
    mDataVGRadio->setFixedHeight(h);

    mDataCalibCheck = new CheckBox(tr("Individual Calib. Dates"));
    mDataCalibCheck->setFixedHeight(h);
    mDataCalibCheck->setChecked(true);

    mWiggleCheck = new CheckBox(tr("Wiggle shifted"));
    mWiggleCheck->setFixedHeight(h);

    mStatCheck = new CheckBox(tr("Show Stat."));
    mStatCheck->setFixedHeight(h);
    mStatCheck->setToolTip(tr("Display numerical results computed on posterior densities below all graphs."));

    QVBoxLayout* resultsGroupLayout = new QVBoxLayout(mEventsPhasesGroup);
    resultsGroupLayout->setContentsMargins(10, 10, 10, 10);
    resultsGroupLayout->setSpacing(15);
    resultsGroupLayout->addWidget(mDataThetaRadio);
    resultsGroupLayout->addWidget(mDataSigmaRadio);
    resultsGroupLayout->addWidget(mDataVGRadio);
    resultsGroupLayout->addWidget(mEventsfoldCheck);
    resultsGroupLayout->addWidget(mDatesfoldCheck);
    resultsGroupLayout->addWidget(mDataCalibCheck);
    resultsGroupLayout->addWidget(mWiggleCheck);
    resultsGroupLayout->addWidget(mStatCheck);

    mEventsPhasesGroup->resize(8 * h, mOptionsW);
    mEventsPhasesGroup->setLayout(resultsGroupLayout);


    // -----------------------------------------------------------------
    //  Tempo Group (if graph list tab = duration)
    // -----------------------------------------------------------------
    mTempoGroup = new QWidget();

    mDurationRadio = new RadioButton(tr("Phase Duration"));
    mDurationRadio->setFixedHeight(h);
    mDurationRadio->setChecked(true);

    mTempoRadio = new RadioButton(tr("Phase Tempo"));
    mTempoRadio->setFixedHeight(h);

    mTempoCredCheck = new CheckBox(tr("Tempo Cred."));
    mTempoCredCheck->setFixedHeight(h);

    mTempoErrCheck = new CheckBox(tr("Tempo Error"));
    mTempoErrCheck->setFixedHeight(h);

    mActivityRadio = new RadioButton(tr("Phase Activity"));
    mActivityRadio->setFixedHeight(h);

    mTempoStatCheck = new CheckBox(tr("Show Tempo Stat."));
    mTempoStatCheck->setFixedHeight(h);
    mTempoStatCheck->setToolTip(tr("Display numerical results computed on posterior densities below all graphs."));

    QVBoxLayout* tempoGroupLayout = new QVBoxLayout(mTempoGroup);
    tempoGroupLayout->setContentsMargins(10, 10, 10, 10);
    tempoGroupLayout->setSpacing(15);
    tempoGroupLayout->addWidget(mDurationRadio);
    tempoGroupLayout->addWidget(mTempoRadio);
    tempoGroupLayout->addWidget(mTempoCredCheck);
    tempoGroupLayout->addWidget(mTempoErrCheck);
    tempoGroupLayout->addWidget(mActivityRadio);
    tempoGroupLayout->addWidget(mTempoStatCheck);
   // mTempoGroup->setLayout(tempoGroupLayout);

    // -----------------------------------------------------------------
    //  Curves Group (if graph list tab = curve)
    // -----------------------------------------------------------------
    mCurvesGroup = new QWidget();
    
    mCurveGRadio = new RadioButton(tr("G"), mCurvesGroup);
    mCurveGRadio->setFixedHeight(h);
    mCurveGRadio->setChecked(true);
    
    mCurveErrorCheck = new CheckBox(tr("Error envelope"), mCurvesGroup);
    mCurveErrorCheck->setFixedHeight(h);
    mCurveErrorCheck->setChecked(true);
    
    mCurveEventsPointsCheck = new CheckBox(tr("Events Points"), mCurvesGroup);
    mCurveEventsPointsCheck->setFixedHeight(h);
    mCurveEventsPointsCheck->setChecked(true);

    mCurveDataPointsCheck = new CheckBox(tr("Data Points"), mCurvesGroup);
    mCurveDataPointsCheck->setFixedHeight(h);
    mCurveDataPointsCheck->setChecked(true);

    mCurveGPRadio = new RadioButton(tr("G Prime"), mCurvesGroup);
    mCurveGPRadio->setFixedHeight(h);
    
    mCurveGSRadio = new RadioButton(tr("G Second"), mCurvesGroup);
    mCurveGSRadio->setFixedHeight(h);
    
    mLambdaRadio = new RadioButton(tr("Lambda"), mCurvesGroup);
    mLambdaRadio->setFixedHeight(h);
    
    mCurveStatCheck = new CheckBox(tr("Show Stat."));
    mCurveStatCheck->setFixedHeight(h);
    mCurveStatCheck->setToolTip(tr("Display numerical results computed on posterior densities below all graphs."));


    QVBoxLayout* curveGroupLayout = new QVBoxLayout(mCurvesGroup);
    curveGroupLayout->setContentsMargins(10, 10, 10, 10);
    curveGroupLayout->addWidget(mCurveGRadio);

    QVBoxLayout* curveOptionGroupLayout = new QVBoxLayout(mCurvesGroup);
    curveOptionGroupLayout->setContentsMargins(15, 0, 0, 0);
   // curveOptionGroupLayout->setSpacing(55);
    curveOptionGroupLayout->addWidget(mCurveErrorCheck, Qt::AlignLeft);
    curveOptionGroupLayout->addWidget(mCurveEventsPointsCheck, Qt::AlignLeft);
    curveOptionGroupLayout->addWidget(mCurveDataPointsCheck, Qt::AlignLeft);

    curveGroupLayout->setSpacing(15);
    curveGroupLayout->addLayout(curveOptionGroupLayout);
    curveGroupLayout->addWidget(mCurveGPRadio);
    curveGroupLayout->addWidget(mCurveGSRadio);
    curveGroupLayout->addWidget(mLambdaRadio);
    curveGroupLayout->addWidget(mCurveStatCheck);

    mCurvesGroup->setLayout(curveGroupLayout);
   // mCurvesGroup->resize(8 * h, mOptionsW);

    // -----------------------------------------------------------------
    //  Connections
    // -----------------------------------------------------------------
    connect(mDataThetaRadio, &RadioButton::clicked, this, &ResultsView::applyCurrentVariable);
    connect(mDataSigmaRadio, &RadioButton::clicked, this, &ResultsView::applyCurrentVariable);
    connect(mDataVGRadio, &RadioButton::clicked, this, &ResultsView::applyCurrentVariable);
    connect(mDurationRadio, &RadioButton::clicked, this, &ResultsView::applyCurrentVariable);
    connect(mTempoRadio, &RadioButton::clicked, this, &ResultsView::applyCurrentVariable);
    connect(mActivityRadio, &RadioButton::clicked, this, &ResultsView::applyCurrentVariable);

    connect(mEventsfoldCheck, &CheckBox::clicked, this, &ResultsView::applyUnfoldEvents);
    connect(mDatesfoldCheck, &CheckBox::clicked, this, &ResultsView::applyUnfoldDates);

    connect(mDataCalibCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mWiggleCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);

    connect(mStatCheck, &CheckBox::clicked, this, &ResultsView::showInfos);
    connect(mTempoStatCheck, &CheckBox::clicked, this, &ResultsView::showInfos);
    connect(mCurveStatCheck, &CheckBox::clicked, this, &ResultsView::showInfos);

    connect(mTempoCredCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mTempoErrCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    
    connect(mCurveGRadio, &CheckBox::clicked, this, &ResultsView::applyCurrentVariable);
    connect(mCurveGPRadio, &CheckBox::clicked, this, &ResultsView::applyCurrentVariable);
    connect(mCurveGSRadio, &CheckBox::clicked, this, &ResultsView::applyCurrentVariable);
    connect(mLambdaRadio, &CheckBox::clicked, this, &ResultsView::applyCurrentVariable);

    connect(mCurveErrorCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mCurveEventsPointsCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mCurveDataPointsCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);

    // -----------------------------------------------------------------
    //  Graph List tab (has to be created after mResultsGroup and mTempoGroup)
    // -----------------------------------------------------------------
    mGraphListTab = new Tabs();
    mGraphListTab->setFixedHeight(mGraphListTab->tabHeight());
    mGraphListTab->addTab( tr("Events"));
    mGraphListTab->addTab(tr("Phases"));
    mGraphListTab->addTab(tr("Tempo"));
    mGraphListTab->addTab(tr("Curve"));

    connect(mGraphListTab, static_cast<void (Tabs::*)(const int&)>(&Tabs::tabClicked), this, &ResultsView::applyGraphListTab);

    // -----------------------------------------------------------------
    //  Tabs : Display / Distrib. Options
    // -----------------------------------------------------------------
    mDisplayDistribTab = new Tabs();
    mDisplayDistribTab->setFixedHeight(mDisplayDistribTab->tabHeight());

    mDisplayDistribTab->addTab(tr("Display"));
    mDisplayDistribTab->addTab(tr("Distrib. Options"));

    // Necessary to reposition all elements inside the selected tab :
    connect(mDisplayDistribTab, static_cast<void (Tabs::*)(const int&)>(&Tabs::tabClicked), this, &ResultsView::applyDisplayTab);

    // -----------------------------------------------------------------
    //  Display Options layout
    // -----------------------------------------------------------------
    mDisplayWidget = new QWidget();
    mSpanGroup  = new QWidget();
    h = 20;

    mSpanTitle = new Label(tr("Span Options"), mDisplayWidget);
    mSpanTitle->setFixedHeight(25);
    mSpanTitle->setIsTitle(true);

    mDisplayStudyBut = new Button(tr("Study Period Display"), mSpanGroup);
    mDisplayStudyBut->setFixedHeight(25);
    mDisplayStudyBut->setToolTip(tr("Restore view with the study period span"));

    mSpanLab = new QLabel(tr("Span"), mSpanGroup);
    mSpanLab->setFixedHeight(h);
    //mSpanLab->setAdjustText(false);

    mCurrentXMinEdit = new LineEdit(mSpanGroup);
    mCurrentXMinEdit->setFixedHeight(h);
    mCurrentXMinEdit->setToolTip(tr("Enter a minimal value to display the curves"));

    mCurrentXMaxEdit = new LineEdit(mSpanGroup);
    mCurrentXMaxEdit->setFixedHeight(h);
    mCurrentXMaxEdit->setToolTip(tr("Enter a maximal value to display the curves"));

    mXLab = new QLabel(tr("X"), mSpanGroup);
    mXLab->setFixedHeight(h);
    mXLab->setAlignment(Qt::AlignCenter);
    //mXLab->setAdjustText(false);

    mXSlider = new QSlider(Qt::Horizontal, mSpanGroup);
    mXSlider->setFixedHeight(h);
    mXSlider->setRange(-100, 100);
    mXSlider->setTickInterval(1);
    mXSlider->setValue(0);

    mXSpin = new QDoubleSpinBox(mSpanGroup);
    mXSpin->setFixedHeight(h);
    mXSpin->setRange(pow(10., double (mXSlider->minimum()/100.)),pow(10., double (mXSlider->maximum()/100.)));
    mXSpin->setSingleStep(.01);
    mXSpin->setDecimals(3);
    mXSpin->setValue(sliderToZoom(mXSlider->value()));
    mXSpin->setToolTip(tr("Enter zoom value to magnify the curves on X span"));

    mMajorScaleLab = new QLabel(tr("Major Interval"), mSpanGroup);
    mMajorScaleLab->setFixedHeight(h);
    mMajorScaleLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mMajorScaleEdit = new LineEdit(mSpanGroup);
    mMajorScaleEdit->setFixedHeight(h);
    mMajorScaleEdit->setText(QString::number(mMajorScale));
    mMajorScaleEdit->setToolTip(tr("Enter a interval for the main division of the axes under the curves, upper than 1"));

    mMinorScaleLab = new Label(tr("Minor Interval Count"), mSpanGroup);
    mMinorScaleLab->setFixedHeight(h);
    mMinorScaleLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mMinorScaleEdit = new LineEdit(mSpanGroup);
    mMinorScaleEdit->setFixedHeight(h);
    mMinorScaleEdit->setText(QString::number(mMinorCountScale));
    mMinorScaleEdit->setToolTip(tr("Enter a interval for the subdivision of the Major Interval for the scale under the curves, upper than 1"));

    connect(mDisplayStudyBut, static_cast<void (Button::*)(bool)>(&Button::clicked), this, &ResultsView::applyStudyPeriod);
    connect(mCurrentXMinEdit, &LineEdit::editingFinished, this, &ResultsView::applyXRange);
    connect(mCurrentXMaxEdit, &LineEdit::editingFinished, this, &ResultsView::applyXRange);
    connect(mXSlider, &QSlider::valueChanged, this, &ResultsView::applyXSlider);
    connect(mXSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &ResultsView::applyXSpin);
    connect(mMajorScaleEdit, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textEdited), this, &ResultsView::applyXScale);
    connect(mMinorScaleEdit, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textEdited), this, &ResultsView::applyXScale);



    QHBoxLayout* spanLayout0 = new QHBoxLayout();
    spanLayout0->setContentsMargins(0, 0, 0, 0);
    spanLayout0->addWidget(mDisplayStudyBut);

    QHBoxLayout* spanLayout1 = new QHBoxLayout();
    spanLayout1->setContentsMargins(0, 0, 0, 0);
    spanLayout1->addWidget(mCurrentXMinEdit);
    spanLayout1->addWidget(mSpanLab);
    spanLayout1->addWidget(mCurrentXMaxEdit);

    QHBoxLayout* spanLayout2 = new QHBoxLayout();
    spanLayout2->setContentsMargins(0, 0, 0, 0);
    spanLayout2->addWidget(mXLab);
    spanLayout2->addWidget(mXSlider);
    spanLayout2->addWidget(mXSpin);

    QHBoxLayout* spanLayout3 = new QHBoxLayout();
    spanLayout3->setContentsMargins(0, 0, 0, 0);
    spanLayout3->addWidget(mMajorScaleLab);
    spanLayout3->addWidget(mMajorScaleEdit);

    QHBoxLayout* spanLayout4 = new QHBoxLayout();
    spanLayout4->setContentsMargins(0, 0, 0, 0);
    spanLayout4->addWidget(mMinorScaleLab);
    spanLayout4->addWidget(mMinorScaleEdit);

    QVBoxLayout* spanLayout = new QVBoxLayout();
    spanLayout->setContentsMargins(10, 10, 10, 10);
    spanLayout->setSpacing(5);
    spanLayout->addLayout(spanLayout0);
    spanLayout->addLayout(spanLayout1);
    spanLayout->addLayout(spanLayout2);
    spanLayout->addLayout(spanLayout3);
    spanLayout->addLayout(spanLayout4);

    mSpanGroup->setLayout(spanLayout);

    // ------------------------------------
    //  Graphic Options
    // ------------------------------------
    mGraphicTitle = new Label(tr("Graphic Options"), mDisplayWidget);
    mGraphicTitle->setFixedHeight(25);
    mGraphicTitle->setIsTitle(true);

    mGraphicGroup = new QWidget();

    mYLab = new QLabel(tr("Y"), mGraphicGroup);
    mYLab->setAlignment(Qt::AlignCenter);
    //mYLab->setAdjustText(false);

    mYSlider = new QSlider(Qt::Horizontal, mGraphicGroup);
    mYSlider->setRange(10, 1000);
    mYSlider->setTickInterval(1);
    mYSlider->setValue(100);

    mYSpin = new QSpinBox(mGraphicGroup);
    mYSpin->setRange(mYSlider->minimum(), mYSlider->maximum());
    mYSpin->setValue(mYSlider->value());
    mYSpin->setToolTip(tr("Enter zoom value to magnify the curves on Y scale"));

    mLabFont = new QLabel(tr("Font"), mGraphicGroup);
    mLabFont->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mFontBut = new Button(font().family() + ", " + QString::number(font().pointSizeF()), mGraphicGroup);
    mFontBut->setFixedHeight(25);
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
    mThicknessCombo->setCurrentIndex(1);

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

    connect(mYSlider, &QSlider::valueChanged, this, &ResultsView::applyYSlider);
    connect(mYSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ResultsView::applyYSpin);
    connect(mFontBut, &QPushButton::clicked, this, &ResultsView::applyFont);
    connect(mThicknessCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ResultsView::applyThickness);
    connect(mOpacityCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ResultsView::applyOpacity);

    QHBoxLayout* graphicLayout1 = new QHBoxLayout();
    graphicLayout1->setContentsMargins(0, 0, 0, 0);
    graphicLayout1->addWidget(mYLab);
    graphicLayout1->addWidget(mYSlider);
    graphicLayout1->addWidget(mYSpin);

    QHBoxLayout* graphicLayout2 = new QHBoxLayout();
    graphicLayout2->setContentsMargins(0, 0, 0, 0);
    graphicLayout2->addWidget(mLabFont);
    graphicLayout2->addWidget(mFontBut);

    QHBoxLayout* graphicLayout3 = new QHBoxLayout();
    graphicLayout3->setContentsMargins(0, 0, 0, 0);
    graphicLayout3->addWidget(mLabThickness);
    graphicLayout3->addWidget(mThicknessCombo);

    QHBoxLayout* graphicLayout4 = new QHBoxLayout();
    graphicLayout4->setContentsMargins(0, 0, 0, 0);
    graphicLayout4->addWidget(mLabOpacity);
    graphicLayout4->addWidget(mOpacityCombo);

    QVBoxLayout* graphicLayout = new QVBoxLayout();
    graphicLayout->setContentsMargins(10, 10, 10, 10);
    graphicLayout->setSpacing(5);
    graphicLayout->addLayout(graphicLayout1);
    graphicLayout->addLayout(graphicLayout2);
    graphicLayout->addLayout(graphicLayout3);
    graphicLayout->addLayout(graphicLayout4);

    mGraphicGroup->setLayout(graphicLayout);

    QVBoxLayout* displayLayout = new QVBoxLayout();
    displayLayout->setContentsMargins(0, 0, 0, 0);
    displayLayout->setSpacing(0);
    displayLayout->addWidget(mSpanTitle);
    displayLayout->addWidget(mSpanGroup);
    displayLayout->addWidget(mGraphicTitle);
    displayLayout->addWidget(mGraphicGroup);
    mDisplayWidget->setLayout(displayLayout);

    // ------------------------------------
    //  MCMC Chains
    //  Note : mChainChecks and mChainRadios are populated by createChainsControls()
    // ------------------------------------
    mChainsTitle = new Label(tr("MCMC Chains"));
    mChainsTitle->setFixedHeight(25);
    mChainsTitle->setIsTitle(true);

    mChainsGroup = new QWidget();

    mAllChainsCheck = new CheckBox(tr("Chain Concatenation"), mChainsGroup);
    mAllChainsCheck->setFixedHeight(16);
    mAllChainsCheck->setChecked(true);

    connect(mAllChainsCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);

    QVBoxLayout* chainsLayout = new QVBoxLayout();
    chainsLayout->setContentsMargins(10, 10, 10, 10);
    chainsLayout->setSpacing(15);
    chainsLayout->addWidget(mAllChainsCheck);
    mChainsGroup->setLayout(chainsLayout);

    // ------------------------------------
    //  Density Options
    // ------------------------------------
    mDensityOptsTitle = new Label(tr("Density Options"));
    mDensityOptsTitle->setFixedHeight(25);
    mDensityOptsTitle->setIsTitle(true);

    mDensityOptsGroup = new QWidget();

    mCredibilityCheck = new CheckBox(tr("Show Confidence Bar"), mDensityOptsGroup);
    mCredibilityCheck->setFixedHeight(16);
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

    mBandwidthSpin = new QDoubleSpinBox(mDensityOptsGroup);
    mBandwidthSpin->setDecimals(2);
    
    connect(mCredibilityCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mFFTLenCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ResultsView::applyFFTLength);
    connect(mBandwidthSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &ResultsView::applyBandwidth);
    connect(mThresholdEdit, &LineEdit::editingFinished, this, &ResultsView::applyThreshold);

    QHBoxLayout* densityLayout0 = new QHBoxLayout();
    densityLayout0->setContentsMargins(0, 0, 0, 0);
    densityLayout0->addWidget(mCredibilityCheck);

    QHBoxLayout* densityLayout1 = new QHBoxLayout();
    densityLayout1->setContentsMargins(0, 0, 0, 0);
    densityLayout1->addWidget(mThreshLab);
    densityLayout1->addWidget(mThresholdEdit);

    QHBoxLayout* densityLayout2 = new QHBoxLayout();
    densityLayout2->setContentsMargins(0, 0, 0, 0);
    densityLayout2->addWidget(mFFTLenLab);
    densityLayout2->addWidget(mFFTLenCombo);

    QHBoxLayout* densityLayout3 = new QHBoxLayout();
    densityLayout3->setContentsMargins(0, 0, 0, 0);
    densityLayout3->addWidget(mBandwidthLab);
    densityLayout3->addWidget(mBandwidthSpin);
    
    QVBoxLayout* densityLayout = new QVBoxLayout();
    densityLayout->setContentsMargins(10, 10, 10, 10);
    densityLayout->setSpacing(5);
    densityLayout->addLayout(densityLayout0);
    densityLayout->addLayout(densityLayout1);
    densityLayout->addLayout(densityLayout2);
    densityLayout->addLayout(densityLayout3);
    mDensityOptsGroup->setLayout(densityLayout);

    // ------------------------------------
    //  Tab Distrib. Options
    // ------------------------------------
    mDistribWidget = new QWidget();

    QVBoxLayout* mcmcLayout = new QVBoxLayout();
    mcmcLayout->setContentsMargins(0, 0, 0, 0);
    mcmcLayout->setSpacing(0);
    mcmcLayout->addWidget(mChainsTitle);
    mcmcLayout->addWidget(mChainsGroup);
    mcmcLayout->addWidget(mDensityOptsTitle);
    mcmcLayout->addWidget(mDensityOptsGroup);
    mDistribWidget->setLayout(mcmcLayout);


    // ------------------------------------
    //  Pagination
    // ------------------------------------
    const int layoutWidth = mOptionsW-2*mMargin;
    const int internSpacing = 1;

    mPageWidget = new QWidget();

    mPreviousPageBut = new Button(tr("Prev."), mPageWidget);
    mPreviousPageBut->setCheckable(false);
    mPreviousPageBut->setFlatHorizontal();
    mPreviousPageBut->setToolTip(tr("Display previous data"));
    mPreviousPageBut->setIconOnly(false);

    mPageEdit = new QLineEdit(mPageWidget);
    mPageEdit->setEnabled(false);
    mPageEdit->setReadOnly(true);
    mPageEdit->setAlignment(Qt::AlignCenter);
    mPageEdit->setText(QString::number(mMaximunNumberOfVisibleGraph));

    mNextPageBut  = new Button(tr("Next"), mPageWidget);
    mNextPageBut->setCheckable(false);
    mNextPageBut->setFlatHorizontal();
    mNextPageBut->setToolTip(tr("Display next data"));
    mNextPageBut->setIconOnly(false);

    mGraphsPerPageLab = new QLabel(tr("Nb Densities / Page"), mPageWidget );
    mGraphsPerPageLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mGraphsPerPageSpin = new QSpinBox(mPageWidget);
    mGraphsPerPageSpin->setRange(1, 100);
    mGraphsPerPageSpin->setValue(mGraphsPerPage);
    mGraphsPerPageSpin->setToolTip(tr("Enter the maximum densities to display on a sheet"));

    /* L'utilisation du QLayout ne fonctionne pas avec mPageEdit et les widgets Button.
     * C'est pourquoi, oil est plus rapide d'utiliser directement setGeometry()
     */
    const int layoutWidthBy3 = (layoutWidth - 2*internSpacing)/3;
    mPreviousPageBut->setGeometry(0, 0, layoutWidthBy3, 33);
    mPageEdit->setGeometry(layoutWidthBy3 + internSpacing, 0, layoutWidthBy3, 33);
    mNextPageBut->setGeometry(2*layoutWidthBy3 + 2*internSpacing, 0, layoutWidthBy3, 33);
    mGraphsPerPageLab->setGeometry(0, 35, layoutWidth/2 - internSpacing/2 , 33);
    mGraphsPerPageSpin->setGeometry(layoutWidth/2 + internSpacing, 35, mGraphsPerPageSpin->width(), 33);

    mPageWidget->setFixedSize(layoutWidth, 66);

    connect(mPreviousPageBut, &Button::pressed, this, &ResultsView::applyPreviousPage);
    connect(mNextPageBut, &Button::pressed, this, &ResultsView::applyNextPage);
    connect(mGraphsPerPageSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ResultsView::applyGraphsPerPage);

    // ------------------------------------
    //  Save Buttons (multiple graphs)
    // ------------------------------------

    mSaveAllWidget = new QWidget();

    mExportImgBut = new Button(tr("Capture"), mSaveAllWidget);
    mExportImgBut->setFlatHorizontal();
    mExportImgBut->setIcon(QIcon(":picture_save.png"));
    mExportImgBut->setToolTip(tr("Save all currently visible results as an image.<br><u>Note</u> : If you want to copy textual results, see the Log tab."));

    mExportResults = new Button(tr("Results"), mSaveAllWidget);
    mExportResults->setFlatHorizontal();
    mExportResults->setIcon(QIcon(":csv.png"));
    mExportResults->setToolTip(tr("Export all result in several files"));

    connect(mExportImgBut, static_cast<void (Button::*)(bool)>(&Button::clicked), this, &ResultsView::exportFullImage);
    connect(mExportResults, static_cast<void (Button::*)(bool)>(&Button::clicked), this, &ResultsView::exportResults);


    QHBoxLayout* mSaveAllLayout = new QHBoxLayout (mSaveAllWidget);
    mSaveAllLayout->setContentsMargins(0, 0, 0, 0);
    mSaveAllLayout->addWidget(mExportImgBut);
    mSaveAllLayout->addSpacing(internSpacing);
    mSaveAllLayout->addWidget(mExportResults);
    mSaveAllWidget->setLayout(mSaveAllLayout);

    // ------------------------------------
    //  Tools Buttons (single graph)
    // ------------------------------------
    mSaveSelectWidget = new QWidget();

    mImageSaveBut = new Button(tr("Save"),mSaveSelectWidget);
    mImageSaveBut->setIcon(QIcon(":picture_save.png"));
    mImageSaveBut->setFlatVertical();
    mImageSaveBut->setToolTip(tr("Save image as file"));

    mImageClipBut = new Button(tr("Copy"), mSaveSelectWidget);
    mImageClipBut->setIcon(QIcon(":clipboard_graph.png"));
    mImageClipBut->setFlatVertical();
    mImageClipBut->setToolTip(tr("Copy image to clipboard"));

    mResultsClipBut = new Button(tr("Copy"), mSaveSelectWidget);
    mResultsClipBut->setIcon(QIcon(":text.png"));
    mResultsClipBut->setFlatVertical();
    mResultsClipBut->setToolTip(tr("Copy text results to clipboard"));

    mDataSaveBut = new Button(tr("Save"), mSaveSelectWidget);
    mDataSaveBut->setIcon(QIcon(":data.png"));
    mDataSaveBut->setFlatVertical();
    mDataSaveBut->setToolTip(tr("Save graph data to file"));

    QHBoxLayout* mSaveSelectLayout = new QHBoxLayout (mSaveSelectWidget);
    mSaveSelectLayout->setContentsMargins(0, 0, 0, 0);
    mSaveSelectLayout->addWidget(mImageSaveBut);
    mSaveSelectLayout->addSpacing(internSpacing);
    mSaveSelectLayout->addWidget(mImageClipBut);
    mSaveSelectLayout->addSpacing(internSpacing);
    mSaveSelectLayout->addWidget(mResultsClipBut);
    mSaveSelectLayout->addSpacing(internSpacing);
    mSaveSelectLayout->addWidget(mDataSaveBut);
    mSaveSelectWidget->setLayout(mSaveSelectLayout);

    connect(mImageSaveBut, static_cast<void (Button::*)(bool)>(&Button::clicked), this, &ResultsView::saveAsImage);
    connect(mImageClipBut, static_cast<void (Button::*)(bool)>(&Button::clicked), this, &ResultsView::imageToClipboard);
    connect(mResultsClipBut, static_cast<void (Button::*)(bool)>(&Button::clicked), this, &ResultsView::resultsToClipboard);
    connect(mDataSaveBut, static_cast<void (Button::*)(bool)>(&Button::clicked), this, &ResultsView::saveGraphData);

    // ------------------------------------
    //  Pagination / Saving
    // ------------------------------------

    mPageSaveTab = new Tabs();
    mPageSaveTab->addTab(tr("Page"));
    mPageSaveTab->addTab(tr("Saving"));
    mPageSaveTab->setFixedHeight(mPageSaveTab->tabHeight());

    mPageSaveTab->setTab(0, false);

    connect(mPageSaveTab, static_cast<void (Tabs::*)(const int&)>(&Tabs::tabClicked), this, &ResultsView::applyPageSavingTab);

    // ---------------------------------------------------------------------------
    //  Right Layout (Options)
    // ---------------------------------------------------------------------------
    mOptionsLayout = new QVBoxLayout(mOptionsWidget);
    mOptionsLayout->setContentsMargins(mMargin, mMargin, 0, 0);
    mOptionsLayout->setSpacing(3);
    mOptionsLayout->addWidget(mGraphListTab);
    mOptionsLayout->addWidget(mEventsPhasesGroup);
    mOptionsLayout->addWidget(mTempoGroup);
    mOptionsLayout->addWidget(mCurvesGroup);

    mOptionsLayout->addWidget(mDisplayDistribTab);
    mOptionsLayout->addWidget(mDisplayWidget);
  //  mOptionsLayout->addWidget(mDistribWidget);

    mOptionsLayout->addWidget(mPageSaveTab);
  //  mOptionsLayout->addSpacing(2);
    mOptionsLayout->addWidget(mPageWidget);

    mOptionsLayout->addStretch();
    mOptionsWidget->setLayout(mOptionsLayout);

    mSaveAllWidget->setParent(mOptionsWidget);
    mSaveSelectWidget->setParent(mOptionsWidget);

    mSaveAllWidget->setVisible(false);
    mSaveSelectWidget->setVisible(false);

    mDistribWidget->setVisible(false);
    // ---------------------------------------------------------------------------
    //  Inititialize tabs indexes
    // ---------------------------------------------------------------------------
    mGraphTypeTabs->setTab(0, false);
    mGraphListTab->setTab(0, false);
    mDisplayDistribTab->setTab(0, false);

    mPageSaveTab->setTab(0, false);

    mEventsScrollArea->setVisible(true);
    mPhasesScrollArea->setVisible(false);
    mTempoScrollArea->setVisible(false);
    mCurveScrollArea->setVisible(false);

    GraphViewResults::mHeightForVisibleAxis = 4 * AppSettings::heigthUnit();
    mGraphHeight = GraphViewResults::mHeightForVisibleAxis;

    mMarker->raise();

    updateOptionsWidget();
}

ResultsView::~ResultsView()
{
    mModel = nullptr;
}

#pragma mark Project & Model

void ResultsView::setProject(Project* project)
{
    /* Starting MCMC calculation does a mModel.clear() at first, and recreate it.
     * Then, it fills its elements (events, ...) with calculated data (trace, ...)
     * If the process is canceled, we only have unfinished data in storage.
     * => The previous nor the new results can be displayed so we must start by clearing the results view! */

    clearResults();
    updateModel(project->mModel);
    connect(project, &Project::mcmcStarted, this, &ResultsView::clearResults);
}

void ResultsView::clearResults()
{
    deleteChainsControls();
    deleteAllGraphsInList(mByEventsGraphs);
    deleteAllGraphsInList(mByPhasesGraphs);
    deleteAllGraphsInList(mByTempoGraphs);
    deleteAllGraphsInList(mByCurveGraphs);
}

void ResultsView::updateModel(Model* model)
{
    if (mModel == nullptr) {
        disconnect(mModel, &Model::newCalculus, this, &ResultsView::generateCurves);
    }

    mModel = model;
    connect(mModel, &Model::newCalculus, this, &ResultsView::generateCurves);

    Scale xScale;
    xScale.findOptimal(mModel->mSettings.mTmin, mModel->mSettings.mTmax, 7);
    mMajorScale = xScale.mark;
    mMinorCountScale = 4;

    mRuler->setRange(mModel->mSettings.getTminFormated(), mModel->mSettings.getTmaxFormated());
    mRuler->setCurrent(mModel->mSettings.getTminFormated(), mModel->mSettings.getTmaxFormated());
    mRuler->setScaleDivision(mMajorScale, mMinorCountScale);

    QLocale locale = QLocale();
    mMajorScaleEdit->setText(locale.toString(mMajorScale));
    mMinorScaleEdit->setText(locale.toString(mMinorCountScale));

    mHasPhases = (mModel->mPhases.size() > 0);

    // ----------------------------------------------------
    //  Create Chains option controls (radio and checkboxes under "MCMC Chains")
    // ----------------------------------------------------
    createChainsControls();

    mCurrentTypeGraph = GraphViewResults::ePostDistrib;
    if (isChronocurve()) {
        mCurrentVariable = GraphViewResults::eG;
        mGraphListTab->setTab(3, false);

    } else
        mCurrentVariable = GraphViewResults::eTheta;

    mFFTLenCombo->setCurrentText(QString::number(mModel->getFFTLength()));
    mBandwidthSpin->setValue(mModel->getBandwidth());
    mThresholdEdit->setText(QString::number(mModel->getThreshold()));
    
    applyStudyPeriod();
    updateOptionsWidget();
    createGraphs();
    updateLayout();
    
    showInfos(false);
}

#pragma mark Layout

void ResultsView::mouseMoveEvent(QMouseEvent* e)
{
    updateMarkerGeometry(e->pos().x());
}

void ResultsView::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    updateLayout();
}

void ResultsView::updateMarkerGeometry(const int x)
{
    const int markerXPos = inRange(0, x, mRuler->x() + mRuler->width());
    mMarker->setGeometry(markerXPos, mGraphTypeTabs->height() + mMargin, mMarker->thickness(), height() - mGraphTypeTabs->height() - mMargin);
}

void ResultsView::updateLayout()
{    
    int leftWidth = width() - mOptionsW - mSbe;
    int graphWidth = leftWidth;
    int tabsH = mGraphTypeTabs->tabHeight();
    int rulerH = Ruler::sHeight;
    int stackH = height() - mMargin - tabsH - rulerH;

    if (mStatCheck->isChecked() || mTempoStatCheck->isChecked() || mCurveStatCheck->isChecked()) {
        graphWidth = (2./3.) * leftWidth;
    }

    // ----------------------------------------------------------
    //  Left layout
    // ----------------------------------------------------------
    mGraphTypeTabs->setGeometry(mMargin, mMargin, leftWidth, tabsH);
    mRuler->setGeometry(0, mMargin + tabsH, graphWidth, rulerH);

    QRect graphScrollGeometry(0, mMargin + tabsH + rulerH, leftWidth, stackH);

    mEventsScrollArea->setGeometry(graphScrollGeometry);
    mPhasesScrollArea->setGeometry(graphScrollGeometry);
    mTempoScrollArea->setGeometry(graphScrollGeometry);
    mCurveScrollArea->setGeometry(graphScrollGeometry);

    updateGraphsLayout();
    updateMarkerGeometry(mMarker->pos().x());


    // --------------------------------------------------------
    //  Right layout
    // --------------------------------------------------------
    mOptionsScroll->setGeometry(leftWidth, 0, mOptionsW, height());
   // mOptionsWidget->setGeometry(0, 0, mOptionsW - sbe, 800);
}


void ResultsView::updateGraphsLayout()
{
    if (mGraphListTab->currentIndex() == 0) {
        updateGraphsLayout(mEventsScrollArea, mByEventsGraphs);

    } else if (mGraphListTab->currentIndex() == 1) {
        updateGraphsLayout(mPhasesScrollArea, mByPhasesGraphs);

    } else if (mGraphListTab->currentIndex() == 2) {
        updateGraphsLayout(mTempoScrollArea, mByTempoGraphs);

    } else if (mGraphListTab->currentIndex() == 3) {
        updateGraphsLayout(mCurveScrollArea, mByCurveGraphs);
    }

    // Display the scroll area corresponding to the selected tab :
    mEventsScrollArea->setVisible(mGraphListTab->currentIndex() == 0);
    mPhasesScrollArea->setVisible(mGraphListTab->currentIndex() == 1);
    mTempoScrollArea->setVisible(mGraphListTab->currentIndex() == 2);
    mCurveScrollArea->setVisible(mGraphListTab->currentIndex() == 3);
}

void ResultsView::updateGraphsLayout(QScrollArea* scrollArea, QList<GraphViewResults*> graphs)
{
    QWidget* widget = scrollArea->widget();

    /*QPalette palette = widget->palette();
    palette.setBrush(QPalette::Background, Qt::blue);
    widget->setPalette(palette);*/

    if (widget) {
        widget->resize(width() - mOptionsW - mSbe, graphs.size() * mGraphHeight);

        for (int i = 0; i<graphs.size(); ++i) {
            graphs[i]->setGeometry(0, i * mGraphHeight, width() - mOptionsW - mSbe, mGraphHeight);
            graphs[i]->setVisible(true);
            graphs[i]->update();
        }
    }
}

#pragma mark Tabs changes listeners

/**
 * This is called when mGraphTypeTabs is clicked.
 * Changing the "graph type" means switching between "Posterior distrib", "History Plot", "Acceptance Rate" and "Autocorrelation".
 * It only makes sense when working on MH variable.
 *
 * Changing it requires the following steps :
 * 1) updateControls : the available options have to be modified according to the graph type
 * 2) generateCurves : no need to call createGraphs because we are only changing the curves displayed in the current graphs list.
 * 3) updateLayout : always needed at the end to refresh the display
*/
void ResultsView::applyGraphTypeTab()
{
    mCurrentTypeGraph = (GraphViewResults::TypeGraph) mGraphTypeTabs->currentIndex();

    updateOptionsWidget();
    generateCurves();
    updateLayout();
}

/**
 * This is called when mGraphListTab is clicked.
 * MCMC display options are only available for mCurrentVariable in : eTheta, eSigma, eDuration
 * so mTabDisplayMCMC must be displayed accordingly
 */
void ResultsView::applyGraphListTab()
{
    int currentIndex = mGraphListTab->currentIndex();
    
    // Display the scroll area corresponding to the selected tab :
    mEventsScrollArea->setVisible(currentIndex == 0);
    mPhasesScrollArea->setVisible(currentIndex == 1);
    mTempoScrollArea->setVisible(currentIndex == 2);
    mCurveScrollArea->setVisible(currentIndex == 3);
    
    // Update the current variable to the most appropriate for this list :
    if (currentIndex == 0) {
        mDataThetaRadio->setChecked(true);

    } else if (currentIndex == 1) {
        mDataThetaRadio->setChecked(true);

    } else if (currentIndex == 2) {
        mDurationRadio->setChecked(true);

    } else if (currentIndex == 3) {
        mCurveGRadio->setChecked(true);
    }

    // Set the current graph type to Posterior distrib :
    mGraphTypeTabs->setTab(0, false);
    mCurrentTypeGraph = (GraphViewResults::TypeGraph) mGraphTypeTabs->currentIndex();
    
    // Changing the graphs list implies to go back to page 1 :
    mCurrentPage = 0;
    
    applyCurrentVariable();

}

void ResultsView::updateCurrentVariable()
{
    if (mGraphListTab->currentIndex() == 0) {
        if (mDataThetaRadio->isChecked()) {
            mCurrentVariable = GraphViewResults::eTheta;

        } else if (mDataSigmaRadio->isChecked()) {
            mCurrentVariable = GraphViewResults::eSigma;

        } else if (mDataVGRadio->isChecked()) {
            mCurrentVariable = GraphViewResults::eVG;
        }

    } else if (mGraphListTab->currentIndex() == 1) {
        if (mDataThetaRadio->isChecked()) {
            mCurrentVariable = GraphViewResults::eTheta;

        } else if (mDataSigmaRadio->isChecked()) {
            mCurrentVariable = GraphViewResults::eSigma;
        }

    } else if (mGraphListTab->currentIndex() == 2) {
        if (mDurationRadio->isChecked()) {
            mCurrentVariable = GraphViewResults::eDuration;

        } else if (mTempoRadio->isChecked()) {
            mCurrentVariable = GraphViewResults::eTempo;

        } else  if (mActivityRadio->isChecked()) {
            mCurrentVariable = GraphViewResults::eActivity;
        }

    } else if (mGraphListTab->currentIndex() == 3) {

        if (mLambdaRadio->isChecked()) {
            mCurrentVariable = GraphViewResults::eLambda;

        } else if (mCurveGRadio->isChecked()) {
            mCurrentVariable = GraphViewResults::eG;
            mCurrentTypeGraph = GraphViewResults::ePostDistrib;
            mGraphTypeTabs->setTab(0, false);

        } else if (mCurveGPRadio->isChecked()) {
            mCurrentVariable = GraphViewResults::eGP;
            mCurrentTypeGraph = GraphViewResults::ePostDistrib;

        } else if (mCurveGSRadio->isChecked()) {
            mCurrentVariable = GraphViewResults::eGS;
            mCurrentTypeGraph = GraphViewResults::ePostDistrib;
            mGraphTypeTabs->setTab(0, false);
        }

    }
}

void ResultsView::applyCurrentVariable()
{
    updateCurrentVariable();
    createGraphs();

    updateOptionsWidget();

    updateLayout();
}

void ResultsView::applyUnfoldEvents()
{
    createGraphs();
    updateOptionsWidget();

    updateLayout();
}

void ResultsView::applyUnfoldDates()
{
    createGraphs();
    updateOptionsWidget();

    updateLayout();
}

void ResultsView::applyDisplayTab()
{
    updateOptionsWidget();
    updateLayout();
}

void ResultsView::applyPageSavingTab()
{
    updateOptionsWidget();
    updateLayout();
}

#pragma mark Chains controls

void ResultsView::toggleDisplayDistrib()
{
    const bool isPostDistrib = isPostDistribGraph();
    // -------------------------------------------------------------------------------------
    //  MCMC Display options are not visible for mCurrentVariable = Tempo or Activity
    // -------------------------------------------------------------------------------------
    if ( mCurrentVariable == GraphViewResults::eTempo ||
         mCurrentVariable == GraphViewResults::eActivity ||
         mCurrentVariable == GraphViewResults::eGP ||
         mCurrentVariable == GraphViewResults::eGS) {

        mDisplayDistribTab->setTabVisible(1, false);
        mDisplayDistribTab->setTab(0, false);

    } else {
        mDisplayDistribTab->setTabVisible(1, true);
    }

    // Search for the visible widget
    QWidget* widFrom = nullptr;
    int widHeigth = 0;

    if (mDisplayWidget->isVisible())
        widFrom = mDisplayWidget;

    else if (mDistribWidget->isVisible())
        widFrom = mDistribWidget;

    // Exchange with the widget corresponding to the requested tab
    if (mDisplayDistribTab->currentName() == tr("Display") ) {
        mDisplayWidget->setVisible(true);
        mDistribWidget->setVisible(false);
        // ------------------------------------
        //  Display Options
        // ------------------------------------
        const  int internSpan = 5;
        const int h = mDisplayStudyBut->height();
        //widHeigth = mSpanTitle->height() + mCurrentXMinEdit->height() + mXSlider->height()
          //          + mMajorScaleEdit->height() + mMinorScaleEdit->height() + 6*internSpan;
        widHeigth = 11*h + 12*internSpan;
        if ( mCurrentVariable == GraphViewResults::eTheta ||
             mCurrentVariable == GraphViewResults::eTempo ||
             mCurrentVariable == GraphViewResults::eActivity ||
             mCurrentVariable == GraphViewResults::eDuration ||
             mCurrentVariable == GraphViewResults::eSigma ||
             mCurrentVariable == GraphViewResults::eVG ||
             mCurrentVariable == GraphViewResults::eG ||
             mCurrentVariable == GraphViewResults::eGP ||
             mCurrentVariable == GraphViewResults::eGS ||
             mCurrentVariable == GraphViewResults::eLambda) {

            mDisplayStudyBut->setText(xScaleRepresentsTime() ? tr("Study Period Display") : tr("Fit Display"));
            mDisplayStudyBut->setVisible(true);
            widHeigth += mDisplayStudyBut->height() + internSpan;
        } else {
            mDisplayStudyBut->setVisible(false);
        }

        mDisplayWidget-> setFixedHeight(widHeigth);
        if (widFrom != mDisplayWidget)
            mOptionsLayout->replaceWidget(widFrom, mDisplayWidget);


    } else { // Tab Distrib. Option
        mDisplayWidget->setVisible(false);
        mDistribWidget->setVisible(true);
        widHeigth = 50;
        // ------------------------------------
        //  MCMC Chains
        //  Switch between checkBoxes or Radio-buttons for chains
        // ------------------------------------
        mAllChainsCheck->setVisible(isPostDistrib);

        if (mAllChainsCheck->isVisible())
            widHeigth += mAllChainsCheck->height();

        for (auto&& checkChain : mChainChecks) {
            checkChain->setVisible(isPostDistrib);
            if (checkChain->isVisible())
                widHeigth += checkChain->height();
        }

        for (auto&& chainRadio : mChainRadios) {
            chainRadio->setVisible(!isPostDistrib);
            if (chainRadio->isVisible())
                widHeigth += chainRadio->height();
        }

        // ------------------------------------
        //  Density Options
        // ------------------------------------
        bool showDensityOptions = isPostDistrib && (mCurrentVariable != GraphViewResults::eTempo
                && mCurrentVariable != GraphViewResults::eG
                && mCurrentVariable != GraphViewResults::eGP
                && mCurrentVariable != GraphViewResults::eGS
                && mCurrentVariable != GraphViewResults::eActivity);

        mDensityOptsTitle->setVisible(showDensityOptions);
     /*   if (mDensityOptsTitle->isVisible())
            widHeigth += mDensityOptsTitle->height();
*/
        mDensityOptsGroup->setVisible(showDensityOptions);
    /*    if (mDensityOptsGroup->isVisible())
            widHeigth += mDensityOptsGroup->height();
            */
        if (widFrom != mDistribWidget)
            mOptionsLayout->replaceWidget(widFrom, mDistribWidget);
    }



}

void ResultsView::createChainsControls()
{
    if (mModel->mChains.size() != mChainChecks.size())
        deleteChainsControls();

    for (int i=0; i<mModel->mChains.size(); ++i) {
        CheckBox* check = new CheckBox(tr("Chain %1").arg(QString::number(i+1)));
        check->setFixedHeight(16);
        check->setVisible(true);
        mChainChecks.append(check);

        connect(check, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);

        RadioButton* radio = new RadioButton(tr("Chain %1").arg(QString::number(i+1)));
        radio->setFixedHeight(16);
        radio->setChecked(i == 0);
        radio->setVisible(true);
        mChainRadios.append(radio);

        connect(radio, &RadioButton::clicked, this, &ResultsView::updateCurvesToShow);

        mChainsGroup->layout()->addWidget(check);
        mChainsGroup->layout()->addWidget(radio);
    }
}

void ResultsView::deleteChainsControls()
{
    for (CheckBox*& check : mChainChecks) {
        disconnect(check, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
        delete check;
    }
    mChainChecks.clear();

    for (RadioButton*& radio : mChainRadios) {
        disconnect(radio, &RadioButton::clicked, this, &ResultsView::updateCurvesToShow);
        delete radio;
    }
    mChainRadios.clear();
}

#pragma mark Graphs UI

void ResultsView::createGraphs()
{
    if (!mModel) {
        return;
    }

    if (mGraphListTab->currentIndex() == 0) {
        createByEventsGraphs();

    } else if (mGraphListTab->currentIndex() == 1) {
        createByPhasesGraphs();

    } else if (mGraphListTab->currentIndex() == 2) {
        createByTempoGraphs();

    } else if (mGraphListTab->currentIndex() == 3) {
        createByCurveGraph();
    }
    
    generateCurves();
}

void ResultsView::updateTotalGraphs()
{
    if (!mModel) {
        mMaximunNumberOfVisibleGraph = 0;
        return;
    }
    
    int totalGraphs = 0;
    
    if (mGraphListTab->currentIndex() == 0) {
        bool showAllEvents = ! mModel->hasSelectedEvents();
        for (auto&& event : mModel->mEvents) {
            if (event->mIsSelected || showAllEvents) {
                ++totalGraphs;
                
                if (mDatesfoldCheck->isChecked()) 
                    totalGraphs += event->mDates.size();

            }
        }
    } else if (mGraphListTab->currentIndex() == 1) {
        bool showAllPhases = ! mModel->hasSelectedPhases();

        for (int i=0; i<mModel->mPhases.size(); ++i) {
            Phase* phase = mModel->mPhases[i];
            if (phase->mIsSelected || showAllPhases) {
                ++totalGraphs;
                
                if (mEventsfoldCheck->isChecked())  {
                    for (auto&& event : phase->mEvents) {
                        ++totalGraphs;

                        if (mDatesfoldCheck->isChecked())
                            totalGraphs += event->mDates.size();

                    }
                }
            }
        }
    } else if (mGraphListTab->currentIndex() == 2) {
        bool showAllPhases = ! mModel->hasSelectedPhases();
        
        for (auto&& phase : mModel->mPhases) {
            if (phase->mIsSelected || showAllPhases) {
                ++totalGraphs;
            }
        }

    } else if (mGraphListTab->currentIndex() == 3) {
        if (mLambdaRadio->isChecked()) {
            ++totalGraphs;

        } else {
            if (!mModel->mEvents.isEmpty()) {
                ModelChronocurve* model = modelChronocurve();
                bool hasY = (model->mChronocurveSettings.mProcessType != ChronocurveSettings::eProcessTypeUnivarie);
                bool hasZ = (model->mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessTypeVectoriel ||
                             model->mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessType3D);

                ++totalGraphs;
                if (hasY) ++totalGraphs;
                if (hasZ) ++totalGraphs;
            }
        }
    }
    
    mMaximunNumberOfVisibleGraph = totalGraphs;
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

    for (auto& event : mModel->mEvents) {
        if (event->mIsSelected || showAllEvents) {
            if (graphIndexIsInCurrentPage(graphIndex)) {
                GraphViewEvent* graph = new GraphViewEvent(eventsWidget);
                graph->setSettings(mModel->mSettings);
                graph->setMCMCSettings(mModel->mMCMCSettings, mModel->mChains);
                graph->setEvent(event);
                graph->setGraphsFont(mFontBut->font());
                graph->setGraphsThickness(mThicknessCombo->currentIndex());
                graph->changeXScaleDivision(mMajorScale, mMinorCountScale);
                graph->setMarginLeft(mMarginLeft);
                graph->setMarginRight(mMarginRight);

                mByEventsGraphs.append(graph);
                connect(graph, &GraphViewResults::selected, this, &ResultsView::togglePageSave);
            }
            ++graphIndex;
                
            if (mDatesfoldCheck->isChecked()) {
                for (auto& date : event->mDates) {
                    if (graphIndexIsInCurrentPage(graphIndex)) {

                        GraphViewDate* graph = new GraphViewDate(eventsWidget);
                        graph->setSettings(mModel->mSettings);
                        graph->setMCMCSettings(mModel->mMCMCSettings, mModel->mChains);
                        graph->setDate(&date);
                        graph->setGraphsFont(mFontBut->font());
                        graph->setGraphsThickness(mThicknessCombo->currentIndex());
                        graph->changeXScaleDivision(mMajorScale, mMinorCountScale);
                        graph->setColor(event->mColor);
                        graph->setGraphsOpacity(mOpacityCombo->currentIndex()*10);
                        graph->setMarginLeft(mMarginLeft);
                        graph->setMarginRight(mMarginRight);

                        mByEventsGraphs.append(graph);
                        connect(graph, &GraphViewResults::selected, this, &ResultsView::togglePageSave);
                    }
                    ++graphIndex;
                }
            }
        }
    }
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

    for (auto& phase : mModel->mPhases) {
        if (phase->mIsSelected || showAllPhases) {
            if (graphIndexIsInCurrentPage(graphIndex)) {
                GraphViewPhase* graph = new GraphViewPhase(phasesWidget);
                graph->setSettings(mModel->mSettings);
                graph->setMCMCSettings(mModel->mMCMCSettings, mModel->mChains);
                graph->setPhase(phase);
                graph->setGraphsFont(mFontBut->font());
                graph->setGraphsThickness(mThicknessCombo->currentIndex());
                graph->changeXScaleDivision(mMajorScale, mMinorCountScale);
                graph->setMarginLeft(mMarginLeft);
                graph->setMarginRight(mMarginRight);

                mByPhasesGraphs.append(graph);
                connect(graph, &GraphViewResults::selected, this, &ResultsView::togglePageSave);
            }
            ++graphIndex;
            
            if (mEventsfoldCheck->isChecked()) {
                for (auto& event : phase->mEvents) {
                     if (graphIndexIsInCurrentPage(graphIndex)) {
                        GraphViewEvent* graph = new GraphViewEvent(phasesWidget);
                        graph->setSettings(mModel->mSettings);
                        graph->setMCMCSettings(mModel->mMCMCSettings, mModel->mChains);
                        graph->setEvent(event);
                        graph->setGraphsFont(mFontBut->font());
                        graph->setGraphsThickness(mThicknessCombo->currentIndex());
                        graph->changeXScaleDivision(mMajorScale, mMinorCountScale);
                        graph->setMarginLeft(mMarginLeft);
                        graph->setMarginRight(mMarginRight);

                        mByPhasesGraphs.append(graph);
                        connect(graph, &GraphViewResults::selected, this, &ResultsView::togglePageSave);
                    }
                    ++graphIndex;
                        
                    if (mDatesfoldCheck->isChecked()) {
                        for (auto& date : event->mDates) {

                            if (graphIndexIsInCurrentPage(graphIndex))  {
                                GraphViewDate* graph = new GraphViewDate(phasesWidget);
                                graph->setSettings(mModel->mSettings);
                                graph->setMCMCSettings(mModel->mMCMCSettings, mModel->mChains);
                                graph->setDate(&date);
                                graph->setGraphsFont(mFontBut->font());
                                graph->setGraphsThickness(mThicknessCombo->currentIndex());
                                graph->changeXScaleDivision(mMajorScale, mMinorCountScale);
                                graph->setColor(event->mColor);
                                graph->setGraphsOpacity(mOpacityCombo->currentIndex()*10);
                                graph->setMarginLeft(mMarginLeft);
                                graph->setMarginRight(mMarginRight);

                                mByPhasesGraphs.append(graph);
                                connect(graph, &GraphViewResults::selected, this, &ResultsView::togglePageSave);
                            }
                            ++graphIndex;
                        }
                    }
                }
            }
        }
    }
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

    for (auto& phase : mModel->mPhases) {
        if (phase->mIsSelected || showAllPhases) {
            if (graphIndexIsInCurrentPage(graphIndex)) {
                GraphViewTempo* graph = new GraphViewTempo(tempoWidget);
                graph->setSettings(mModel->mSettings);
                graph->setMCMCSettings(mModel->mMCMCSettings, mModel->mChains);
                graph->setPhase(phase);
                graph->setGraphsFont(mFontBut->font());
                graph->setGraphsThickness(mThicknessCombo->currentIndex());
                graph->changeXScaleDivision(mMajorScale, mMinorCountScale);
                graph->setMarginLeft(mMarginLeft);
                graph->setMarginRight(mMarginRight);

                mByTempoGraphs.append(graph);
                connect(graph, &GraphViewResults::selected, this, &ResultsView::togglePageSave);
            }
            ++graphIndex;
        }
    }
}

void ResultsView::createByCurveGraph()
{
    Q_ASSERT(isChronocurve());
//if (!isChronocurve())
  //  return;
    ModelChronocurve* model = modelChronocurve();
    
    // ----------------------------------------------------------------------
    //  Disconnect and delete existing graphs
    // ----------------------------------------------------------------------
    deleteAllGraphsInList(mByCurveGraphs);
    
    QWidget* widget = mCurveScrollArea->widget();

    if (mLambdaRadio->isChecked())  {
        GraphViewAlpha* graphAlpha = new GraphViewAlpha(widget);
        graphAlpha->setSettings(mModel->mSettings);
        graphAlpha->setMCMCSettings(mModel->mMCMCSettings, mModel->mChains);
        graphAlpha->setGraphsFont(mFontBut->font());
        graphAlpha->setGraphsThickness(mThicknessCombo->currentIndex());
        graphAlpha->changeXScaleDivision(mMajorScale, mMinorCountScale);
        graphAlpha->setMarginLeft(mMarginLeft);
        graphAlpha->setMarginRight(mMarginRight);
        graphAlpha->setTitle(tr("Lambda Spline"));
        graphAlpha->setModel(model);
        

        QString resultsText = ModelUtilities::curveResultsText(model);
        QString resultsHTML = ModelUtilities::curveResultsHTML(model);
        graphAlpha->setNumericalResults(resultsHTML, resultsText);

        mByCurveGraphs.append(graphAlpha);
        connect(graphAlpha, &GraphViewResults::selected, this, &ResultsView::togglePageSave);

    } else  {
        bool hasY = (model->mChronocurveSettings.mProcessType != ChronocurveSettings::eProcessTypeUnivarie);
        bool hasZ = (model->mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessTypeVectoriel ||
                     model->mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessType3D);

        // insert refpoints for X
        const double thresh = 68.4; //80;
        QVector<RefPoint> eventsPts;
        QVector<RefPoint> dataPts;


        for (auto& event : modelChronocurve()->mEvents) {
            RefPoint evPts;
            RefPoint dPts;

            // Set Y
            if (!hasY) {
                switch (model->mChronocurveSettings.mVariableType) {
                case ChronocurveSettings::eVariableTypeInclinaison :
                    evPts.Ymean = event-> mYInc;
                    evPts.Yerr = event->mSInc;
                    break;
                case ChronocurveSettings::eVariableTypeDeclinaison :
                    evPts.Ymean = event-> mYDec;
                    evPts.Yerr = event->mSInc / cos(event->mYInc * M_PI /180.);
                    break;
                case ChronocurveSettings::eVariableTypeIntensite :
                    evPts.Ymean = event-> mYInt;
                    evPts.Yerr = event->mSInt;
                    break;
                case ChronocurveSettings::eVariableTypeProfondeur :
                    evPts.Ymean = event-> mYInt;
                    evPts.Yerr = event->mSInt;
                    break;
                case ChronocurveSettings::eVariableTypeAutre :
                    evPts.Ymean = event-> mYInt;
                    evPts.Yerr = event->mSInt;
                    break;
                }

            } else { //must be inclination
                evPts.Ymean = event-> mYInc;
                evPts.Yerr = event->mSInc;
            }
            evPts.color = event->mColor;

            // Set X = time
            if (event->mType == Event::eDefault) {

                double tmin = HUGE_VAL;
                double tmax = -HUGE_VAL;

                for (auto&& date: event->mDates) {
                    double dataTmin = HUGE_VAL;
                    double dataTmax = -HUGE_VAL;
                    QMap<double, double> calibMap = date.getRawCalibMap();//  getFormatedCalibMap();

                    QMap<double, double> hpd;
                    QMap<double, double> subData = getMapDataInRange(calibMap, mModel->mSettings.mTmin, mModel->mSettings.mTmax);// mSettings.getTminFormated(), mSettings.getTmaxFormated());

                    if (subData.size() == 0) {
                        continue;
                    } else if (subData.size() == 1) {
                        hpd = subData;

                    } else {
                         hpd = QMap<double, double>(create_HPD(subData, thresh));
                    }


                    QMapIterator<double, double> it(hpd);
                    it.toFront();
                    while (it.hasNext()) {
                        it.next();
                        if (it.value() != 0) {
                            dataTmin = std::min(dataTmin, it.key());
                            break;
                        }
                    }
                    it.toBack();
                    while (it.hasPrevious()) {
                        it.previous();
                        if (it.value() != 0) {
                            dataTmax = std::max(dataTmax, it.key());
                            break;
                        }
                    }
                    tmin = std::min(tmin, dataTmin);
                    tmax = std::max(tmax, dataTmax);

                    dPts.Xmean = (dataTmax + dataTmin) / 2.;
                    dPts.Xerr = (dataTmax - dataTmin)/2.;
                    dPts.Ymean = evPts.Ymean;
                    dPts.Yerr = evPts.Yerr;
                    dPts.color = event->mColor;


                    // memo Data Points
                    dataPts.append(dPts);
                }
                double tmoy = (tmax + tmin) / 2.;
                tmoy = DateUtils::convertToAppSettingsFormat(tmoy);
                const double terr = (tmax - tmin) / 2.;

                evPts.Xmean = tmoy;
                evPts.Xerr = terr;

            } else {
                evPts.Xmean = event->mTheta.mX; // always the same value
                evPts.Xerr = 0.;

                dPts.Xmean = event->mTheta.mX; // always the same value
                dPts.Xerr = 0;

                dPts.Ymean = evPts.Ymean;
                dPts.Yerr = evPts.Yerr;
                dPts.color = event->mColor;

                // memo Data Points
                dataPts.append(dPts);
            }

            // memo Events Points
            eventsPts.append(evPts);

        }

        GraphViewCurve* graphX = new GraphViewCurve(widget);
        graphX->setSettings(mModel->mSettings);
        graphX->setMCMCSettings(mModel->mMCMCSettings, mModel->mChains);
        graphX->setGraphsFont(mFontBut->font());
        graphX->setGraphsThickness(mThicknessCombo->currentIndex());
        graphX->changeXScaleDivision(mMajorScale, mMinorCountScale);
        graphX->setMarginLeft(mMarginLeft);
        graphX->setMarginRight(mMarginRight);


        QString resultsText = ModelUtilities::curveResultsText(model);
        QString resultsHTML = ModelUtilities::curveResultsHTML(model);
        graphX->setNumericalResults(resultsHTML, resultsText);

        if (model->mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessTypeUnivarie ) {
            switch (model->mChronocurveSettings.mVariableType) {
                 case ChronocurveSettings::eVariableTypeInclinaison :
                      if (mCurrentVariable == GraphViewResults::eGP) {
                         graphX->setTitle(tr("Speed Inclination"));
                      } else if (mCurrentVariable == GraphViewResults::eGS) {
                          graphX->setTitle(tr("Acceleration Inclination"));
                       } else {
                          graphX->setTitle(tr("Inclination"));
                      }
                      break;

                 case ChronocurveSettings::eVariableTypeDeclinaison :
                       if (mCurrentVariable == GraphViewResults::eGP) {
                          graphX->setTitle(tr("Speed Declination"));
                       } else if (mCurrentVariable == GraphViewResults::eGS) {
                           graphX->setTitle(tr("Acceleration Declination"));
                        } else {
                           graphX->setTitle(tr("Declination"));
                       }
                       break;

                 case ChronocurveSettings::eVariableTypeIntensite:
                      if (mCurrentVariable == GraphViewResults::eGP) {
                         graphX->setTitle(tr("Speed Field"));
                      } else if (mCurrentVariable == GraphViewResults::eGS) {
                          graphX->setTitle(tr("Acceleration Field"));
                       } else {
                          graphX->setTitle(tr("Field"));
                      }
                      break;

                 case ChronocurveSettings::eVariableTypeProfondeur:
                      if (mCurrentVariable == GraphViewResults::eGP) {
                         graphX->setTitle(tr("Speed Depth"));
                      } else if (mCurrentVariable == GraphViewResults::eGS) {
                          graphX->setTitle(tr("Acceleration Depth"));
                       } else {
                          graphX->setTitle(tr("Depth"));
                      }
                      break;

                 case ChronocurveSettings::eVariableTypeAutre:
                       if (mCurrentVariable == GraphViewResults::eGP) {
                          graphX->setTitle(tr("Speed"));
                       } else if (mCurrentVariable == GraphViewResults::eGS) {
                           graphX->setTitle(tr("Acceleration"));
                        } else {
                           graphX->setTitle(tr("Measure"));
                       }
                       break;
            }

        } else if (model->mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessTypeSpherique ||
                   model->mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessTypeVectoriel) {
            if (mCurrentVariable == GraphViewResults::eGP) {
               graphX->setTitle(tr("Speed Inclination"));
            } else if (mCurrentVariable == GraphViewResults::eGS) {
                graphX->setTitle(tr("Acceleration Inclination"));
             } else {
                graphX->setTitle(tr("Inclination"));
            }

        } else if (model->mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessType3D ) {
            if (mCurrentVariable == GraphViewResults::eGP) {
               graphX->setTitle(tr("Speed X"));
            } else if (mCurrentVariable == GraphViewResults::eGS) {
                graphX->setTitle(tr("Acceleration X"));
             } else {
                graphX->setTitle(tr("X"));
            }
        }

        graphX->setComposanteG(modelChronocurve()->mPosteriorMeanG.gx);
        graphX->setComposanteGChains(modelChronocurve()->getChainsMeanGComposanteX());
        graphX->setEvents(modelChronocurve()->mEvents);
        graphX->setEventsPoints(eventsPts);
        graphX->setDataPoints(dataPts);


        mByCurveGraphs.append(graphX);
        
        connect(graphX, &GraphViewResults::selected, this, &ResultsView::togglePageSave);
        
        if (hasY) {
            GraphViewCurve* graphY = new GraphViewCurve(widget);
            graphY->setSettings(mModel->mSettings);
            graphY->setMCMCSettings(mModel->mMCMCSettings, mModel->mChains);
            graphY->setGraphsFont(mFontBut->font());
            graphY->setGraphsThickness(mThicknessCombo->currentIndex());
            graphY->changeXScaleDivision(mMajorScale, mMinorCountScale);
            graphY->setMarginLeft(mMarginLeft);
            graphY->setMarginRight(mMarginRight);

            if (model->mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessTypeSpherique ||
                               model->mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessTypeVectoriel) {

                if (mCurrentVariable == GraphViewResults::eGP) {
                    graphY->setTitle(tr("Speed Declination"));
                } else if (mCurrentVariable == GraphViewResults::eGS) {
                    graphY->setTitle(tr("Acceleration Declination"));
                } else {
                    graphY->setTitle(tr("Declination"));
                }
            } else if (model->mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessType3D ) {

                if (mCurrentVariable == GraphViewResults::eGP) {
                    graphY->setTitle(tr("Speed Y"));
                } else if (mCurrentVariable == GraphViewResults::eGS) {
                    graphY->setTitle(tr("Acceleration Y"));
                } else {
                    graphY->setTitle(tr("Y"));
                }
            }

            graphY->setComposanteG(modelChronocurve()->mPosteriorMeanG.gy);
            graphY->setComposanteGChains(modelChronocurve()->getChainsMeanGComposanteY());

            graphY->setEvents(modelChronocurve()->mEvents);
            // change the values of the Y and the error, with the values of the declination and the error, we keep tmean
            int i = 0;
            for (auto& event : modelChronocurve()->mEvents) {
                eventsPts[i].Ymean = event-> mYDec;
                eventsPts[i].Yerr = event->mSInc / cos(event->mYInc * M_PI /180.);
               // eventsPts[i].color = event->mColor;

                for (int iData = 0 ; iData < event->mDates.size(); ++iData) {
                    dataPts[iData + i].Ymean = eventsPts.at(i).Ymean;
                    dataPts[iData + i].Yerr = eventsPts.at(i).Ymean;
                  //  dataPts[iData + i].color = event->mColor;
                  }


                ++i;
            }
            graphY->setEventsPoints(eventsPts);
            mByCurveGraphs.append(graphY);
            
            connect(graphY, &GraphViewResults::selected, this, &ResultsView::togglePageSave);
        }
        
        if (hasZ) {
            GraphViewCurve* graphZ = new GraphViewCurve(widget);
            graphZ->setSettings(mModel->mSettings);
            graphZ->setMCMCSettings(mModel->mMCMCSettings, mModel->mChains);
            graphZ->setGraphsFont(mFontBut->font());
            graphZ->setGraphsThickness(mThicknessCombo->currentIndex());
            graphZ->changeXScaleDivision(mMajorScale, mMinorCountScale);
            graphZ->setMarginLeft(mMarginLeft);
            graphZ->setMarginRight(mMarginRight);

            if (model->mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessTypeVectoriel ) {
                if (mCurrentVariable == GraphViewResults::eGP) {
                    graphZ->setTitle(tr("Speed Field"));

                } else if (mCurrentVariable == GraphViewResults::eGS) {
                    graphZ->setTitle(tr("Acceleration Field"));

                } else {
                    graphZ->setTitle(tr("Field"));
                }
            
            } else if (model->mChronocurveSettings.mProcessType == ChronocurveSettings::eProcessType3D ) {

                if (mCurrentVariable == GraphViewResults::eGP) {
                    graphZ->setTitle(tr("Speed Z"));
                } else if (mCurrentVariable == GraphViewResults::eGS) {
                    graphZ->setTitle(tr("Acceleration Z"));
                } else {
                    graphZ->setTitle(tr("Z"));
                }
            }

            graphZ->setComposanteG(modelChronocurve()->mPosteriorMeanG.gz);
            graphZ->setComposanteGChains(modelChronocurve()->getChainsMeanGComposanteZ());

            graphZ->setEvents(modelChronocurve()->mEvents);
            int i = 0;
            for (auto& event : modelChronocurve()->mEvents) {
                eventsPts[i].Ymean = event-> mYInt;
                eventsPts[i].Yerr = event->mSInt;

                for (int iData = 0 ; iData < event->mDates.size(); ++iData) {
                    dataPts[iData + i].Ymean = eventsPts.at(i).Ymean;
                    dataPts[iData + i].Yerr = eventsPts.at(i).Ymean;
                    //dataPts[iData + i].color = event->mColor;
                  }


                ++i;
            }
            graphZ->setEventsPoints(eventsPts);
            mByCurveGraphs.append(graphZ);
            
            connect(graphZ, &GraphViewResults::selected, this, &ResultsView::togglePageSave);
        }
    }
}

void ResultsView::deleteAllGraphsInList(QList<GraphViewResults*>& list)
{
    for (auto&& graph : list) {
        //disconnect(graph, &GraphViewResults::selected, this, &ResultsView::togglePageSave);
        disconnect(graph, nullptr, nullptr, nullptr); //Disconnect everything connected to
        delete graph;
    }
    list.clear();
}

QList<GraphViewResults*> ResultsView::allGraphs()
{
    QList<GraphViewResults*> graphs;
    graphs.append(mByEventsGraphs);
    graphs.append(mByPhasesGraphs);
    graphs.append(mByTempoGraphs);
    graphs.append(mByCurveGraphs);
    return graphs;
}

bool ResultsView::hasSelectedGraphs()
{
    return (currentGraphs(true).size() > 0);
}

QList<GraphViewResults*> ResultsView::currentGraphs(bool onlySelected)
{
    QList<GraphViewResults*> graphs;
    QList<GraphViewResults*> byGraphs;

    switch (mGraphListTab->currentIndex()) {
    case 0 :
        byGraphs = mByEventsGraphs;
        break;
    case 1 :
        byGraphs = mByPhasesGraphs;
        break;
    case 2 :
        byGraphs = mByTempoGraphs;
        break;
    case 3 :
        byGraphs = mByCurveGraphs;
        break;
    default:
        byGraphs = QList<GraphViewResults*>();

    }

    if (onlySelected && !byGraphs.isEmpty()) {
        for (auto&& graph : byGraphs)
            if (graph->isSelected())
                graphs.append(graph);

    } else {
        return byGraphs;
    }

/*    if (mGraphListTab->currentIndex() == 0) {
        for (auto&& graph : mByEventsGraphs) {
            if (!onlySelected || graph->isSelected()) {
                graphs.append(graph);
            }
        }

    } else if (mGraphListTab->currentIndex() == 1) {
        for (auto&& graph : mByPhasesGraphs) {
            if (!onlySelected || graph->isSelected()) {
                graphs.append(graph);
            }
        }

    } else if (mGraphListTab->currentIndex() == 2) {
        for (auto&& graph : mByTempoGraphs) {
            if (!onlySelected || graph->isSelected()) {
                graphs.append(graph);
            }
        }

    } else if (mGraphListTab->currentIndex() == 3) {
        for (auto&& graph : mByCurveGraphs) {
            if (!onlySelected || graph->isSelected()) {
                graphs.append(graph);
            }
        }
    }
*/


    return graphs;
}


#pragma mark Pagination


bool ResultsView::graphIndexIsInCurrentPage(int graphIndex)
{
    int firstIndexToShow = mCurrentPage * mGraphsPerPage;
    return (graphIndex >= firstIndexToShow) && (graphIndex < (firstIndexToShow + mGraphsPerPage));
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
    // -----------------------------------------------------------------
    //  Generate all graphs curves in the current list
    // -----------------------------------------------------------------
    QList<GraphViewResults*> listGraphs = currentGraphs(false);
    for (GraphViewResults*& graph : listGraphs) {
        graph->generateCurves(GraphViewResults::TypeGraph(mCurrentTypeGraph), mCurrentVariable);
    }
    
    updateCurvesToShow();
    updateGraphsMinMax();
    updateScales();
}

void ResultsView::updateGraphsMinMax()
{
    QList<GraphViewResults*> listGraphs = currentGraphs(false);
    if (mCurrentTypeGraph == GraphViewResults::ePostDistrib) {
        if (mCurrentVariable == GraphViewResults::eSigma) {
            mResultMinX = 0.;
            mResultMaxX = getGraphsMax(listGraphs, "Sigma", 100.);

        } else if (mCurrentVariable == GraphViewResults::eDuration) {
            mResultMinX = 0.;
            mResultMaxX = getGraphsMax(listGraphs, "Post Distrib Duration", 100.);

        }else if (mCurrentVariable == GraphViewResults::eVG) {
            mResultMinX = getGraphsMin(listGraphs, "Std G", 0.);
            mResultMaxX = getGraphsMax(listGraphs, "Std G", 100.);

        } else if (mCurrentVariable == GraphViewResults::eLambda) {
            mResultMinX = getGraphsMin(listGraphs, "Lambda", -20.);
            mResultMaxX = getGraphsMax(listGraphs, "Lambda", 20.);

        } else {
            mResultMinX = mModel->mSettings.getTminFormated();
            mResultMaxX = mModel->mSettings.getTmaxFormated();
        }

     } else if ((mCurrentTypeGraph == GraphViewResults::eTrace) || (mCurrentTypeGraph == GraphViewResults::eAccept)) {
            for (int i=0; i<mChainRadios.size(); ++i) {
                if (mChainRadios.at(i)->isChecked()) {
                    const ChainSpecs& chain = mModel->mChains.at(i);
                    mResultMinX = 0;
                    const int adaptSize = chain.mBatchIndex * chain.mIterPerBatch;
                    const int runSize = chain.mRealyAccepted;
                    mResultMaxX = 1 + adaptSize + runSize;
                    break;
                }
            }

    } else if (mCurrentTypeGraph == GraphViewResults::eCorrel) {
        mResultMinX = 0.;
        mResultMaxX = 40;

    }
}

double ResultsView::getGraphsMax(const QList<GraphViewResults*>& graphs, const QString& title, double maxFloor)
{
    double max = 0.;
    
    QList<GraphViewResults*>::const_iterator it = graphs.cbegin();
    QList<GraphViewResults*>::const_iterator itEnd = graphs.cend();

    while (it != itEnd)  {
        GraphViewResults* graphWrapper = (*it);
        QList<GraphCurve> curves = graphWrapper->getGraph()->getCurves();
        for (auto&& curve : curves) {
              if (curve.mName.contains(title) && (curve.mVisible == true)) {
                max = ceil(std::max(max, curve.mData.lastKey()));
            }
        }
        ++it;
    }
    return std::max(maxFloor, max);
}

double ResultsView::getGraphsMin(const QList<GraphViewResults*>& graphs, const QString& title, double minFloor)
{
    double min = 0.;

    QList<GraphViewResults*>::const_iterator it = graphs.cbegin();
    QList<GraphViewResults*>::const_iterator itEnd = graphs.cend();

    while (it != itEnd)  {
        GraphViewResults* graphWrapper = (*it);
        QList<GraphCurve> curves = graphWrapper->getGraph()->getCurves();
        for (auto&& curve : curves) {
            if (curve.mName.contains(title) && (curve.mVisible == true)) {
                min = floor(std::min(min, curve.mData.firstKey()));
            }
        }
        ++it;
    }

    return std::min(minFloor, min);
}

/**
 *  @brief Decide which curve graphs must be show, based on currently selected options.
 *  @brief This function does NOT remove or create any curve in graphs! It only checks if existing curves should be visible or not.
 */
void ResultsView::updateCurvesToShow()
{
    // --------------------------------------------------------
    //  Gather selected chain options
    // --------------------------------------------------------
    bool showAllChains = mAllChainsCheck->isChecked();

    // --------------------------------------------------------
    //  showChainList is a list of booleans describing which chains are visible or not.
    //  For Post distribs, multiple chains can be visible at once (checkboxes)
    //  In other cases, only one chain can be displayed (radios)
    // --------------------------------------------------------
    QList<bool> showChainList;
    if (mCurrentTypeGraph == GraphViewResults::ePostDistrib) {
        for (CheckBox*& cbButton : mChainChecks) {
            showChainList.append(cbButton->isChecked());
        }

    } else {
        for (RadioButton*& rButton : mChainRadios) {
            showChainList.append(rButton->isChecked());
        }
    }

    // --------------------------------------------------------
    //  Find the currently selected list of graphs
    // --------------------------------------------------------
    QList<GraphViewResults*> listGraphs = currentGraphs(false);

    // --------------------------------------------------------
    //  Options for "Curve"
    // --------------------------------------------------------
    if ((mGraphListTab->currentIndex() == 3) && !mLambdaRadio->isChecked()) {
        const bool showG = mCurveGRadio->isChecked();
        const bool showGError = showG && mCurveErrorCheck->isChecked();
        const bool showEventsPoints = showG && mCurveEventsPointsCheck->isChecked();
        const bool showDataPoints = mCurveDataPointsCheck->isChecked();
        const bool showGP = mCurveGPRadio->isChecked();
        const bool showGS = mCurveGSRadio->isChecked();
        const bool showStat = mCurveStatCheck->isChecked();
        
        // --------------------------------------------------------
        //  Update Graphs with selected options
        // --------------------------------------------------------
        for (GraphViewResults*& graph : listGraphs) {
            GraphViewCurve* graphCurve = static_cast<GraphViewCurve*>(graph);
            graphCurve->setShowNumericalResults(showStat);
            graphCurve->updateCurvesToShowForG(showAllChains, showChainList, showG, showGError, showEventsPoints, showDataPoints, showGP, showGS);
        }
    }
    // --------------------------------------------------------
    //  All others
    // --------------------------------------------------------
    else {
        bool showCalib = mDataCalibCheck->isChecked();
        const bool showWiggle = mWiggleCheck->isChecked();
        bool showCredibility = mCredibilityCheck->isChecked();
        bool showStat = mStatCheck->isChecked();

        if (mCurrentVariable == GraphViewResults::eTempo) {
            showCalib = mTempoErrCheck->isChecked();
            showCredibility = mTempoCredCheck->isChecked();

        } else if (mCurrentVariable == GraphViewResults::eLambda){
            showStat = mCurveStatCheck->isChecked();
        }

        // --------------------------------------------------------
        //  Update Graphs with selected options
        // --------------------------------------------------------
        for (GraphViewResults*& graph : listGraphs) {
            graph->setShowNumericalResults(showStat);
            graph->updateCurvesToShow(showAllChains, showChainList, showCredibility, showCalib, showWiggle);
        }
    }
}

/**
 *  @brief
 *  This method does the following :
 *  - Defines [mResultMinX, mResultMaxX]
 *  - Defines [mResultCurrentMinX, mResultCurrentMaxX] (based on saved zoom if any)
 *  - Computes mResultZoomX
 *  - Set Ruler Areas
 *  - Set Ruler and graphs range and zoom
 *  - Update mXMinEdit, mXMaxEdit, mXSlider, mXSpin, mMajorScaleEdit, mMinorScaleEdit
 */
void ResultsView::updateScales()
{
    if (!mModel) {
        return;
    }

    ProjectSettings s = mModel->mSettings;


    // ------------------------------------------------------------------
    //  Define mResultCurrentMinX and mResultCurrentMaxX
    //  + Restore last zoom values if any
    // ------------------------------------------------------------------

    // The key of the saved zooms map is as long as that :
    QPair<GraphViewResults::Variable, GraphViewResults::TypeGraph> key(mCurrentVariable, mCurrentTypeGraph);

    // Anyway, let's check if we have a saved zoom value for this key :
    if (mZooms.find(key) != mZooms.end()) {
        // Get the saved (unformatted) values
        double tMin = mZooms.value(key).first;
        double tMax = mZooms.value(key).second;

        // These graphs needs formating since the x axis represents the time
        if (xScaleRepresentsTime()) {
            double tMinFormatted = DateUtils::convertToAppSettingsFormat(tMin);
            double tMaxFormatted = DateUtils::convertToAppSettingsFormat(tMax);

            // Min and max may be inverted due to formatting, so we use std::minmax
            std::pair<double, double> currentMinMax = std::minmax(tMinFormatted, tMaxFormatted);

            mResultCurrentMinX = currentMinMax.first;
            mResultCurrentMaxX = currentMinMax.second;

        } else {
            mResultCurrentMinX = tMin;
            mResultCurrentMaxX = tMax;
        }

    } else {
        if ((mCurrentTypeGraph == GraphViewResults::eTrace) || (mCurrentTypeGraph == GraphViewResults::eAccept)) {
            for (int i=0; i<mChainRadios.size(); ++i) {
                if (mChainRadios.at(i)->isChecked()) {
                    const ChainSpecs& chain = mModel->mChains.at(i);
                    mResultCurrentMinX = 0;
                    const int adaptSize = chain.mBatchIndex * chain.mIterPerBatch;
                    const int runSize = chain.mRealyAccepted;
                    mResultCurrentMaxX = 1 + chain.mIterPerBurn + adaptSize + runSize;
                    break;
                }
            }
        } else if (mCurrentTypeGraph == GraphViewResults::eCorrel) {
            mResultCurrentMinX = 0;
            mResultCurrentMaxX = 40;

        } else {
            mResultCurrentMinX = mResultMinX;
            mResultCurrentMaxX = mResultMaxX;
        }


    }
    
    // Now, let's check if we have a saved scale (ticks interval) value for this key :
    if (mScales.find(key) != mScales.end()) {
        mMajorScale = mScales.value(key).first;
        mMinorCountScale = mScales.value(key).second;

    } else {
        // For correlation graphs, ticks intervals are not an available option
        if (mCurrentTypeGraph == GraphViewResults::eCorrel) {
            mMajorScale = 10.;
            mMinorCountScale = 10;
        }
        // All other cases (default behavior)
        else {
            Scale xScale;
            xScale.findOptimal(mResultCurrentMinX, mResultCurrentMaxX, 10);
            mMajorScale = xScale.mark;
            mMinorCountScale = 10;
        }
    }

    // ------------------------------------------------------------------
    //  Compute mResultZoomX
    // ------------------------------------------------------------------
    mResultZoomX = (mResultMaxX - mResultMinX) / (mResultCurrentMaxX - mResultCurrentMinX);

    // ------------------------------------------------------------------
    //  Set Ruler Areas (Burn, Adapt, Run)
    // ------------------------------------------------------------------
    mRuler->clearAreas();

    if ( xScaleRepresentsTime() ) {
        // The X zoom uses a log scale on the spin box and can be controlled by the linear slider
         mXSlider->setRange(-100, 100);
         mXSpin->setRange(sliderToZoom(-100), sliderToZoom(100));
         mXSpin->setSingleStep(.01);
         mXSpin->setDecimals(3);

         // The Ruler range is much wider based on the minimal zoom
         const double tCenter = (mResultCurrentMinX + mResultCurrentMaxX) / 2.;
         const double tSpan = mResultCurrentMaxX - mResultCurrentMinX;
         const double tRangeMin = tCenter - ((tSpan/2.) / sliderToZoom(mXSlider->minimum()));
         const double tRangeMax = tCenter + ((tSpan/2.) / sliderToZoom(mXSlider->minimum()));

         mRuler->setRange(tRangeMin, tRangeMax);
         mRuler->setFormatFunctX(nullptr);

    } else if (mCurrentTypeGraph == GraphViewResults::ePostDistrib &&
               ( mCurrentVariable == GraphViewResults::eSigma ||
                        mCurrentVariable == GraphViewResults::eDuration) ) {

                // The X zoom uses a log scale on the spin box and can be controlled by the linear slider
                mXSlider->setRange(-100, 100);
                mXSpin->setRange(sliderToZoom(-100), sliderToZoom(100));
                mXSpin->setSingleStep(.01);
                mXSpin->setDecimals(3);

                // The Ruler range is much wider based on the minimal zoom
                const double tRangeMax = mResultMaxX / sliderToZoom(mXSlider->minimum());

                mRuler->setRange(0, tRangeMax);
                mRuler->setFormatFunctX(nullptr);



    } else if (mCurrentTypeGraph == GraphViewResults::ePostDistrib &&
                       ( mCurrentVariable == GraphViewResults::eVG ||
                        mCurrentVariable == GraphViewResults::eLambda) ){

                // The X zoom uses a log scale on the spin box and can be controlled by the linear slider
                mXSlider->setRange(-100, 100);
                mXSpin->setRange(sliderToZoom(-100), sliderToZoom(100));
                mXSpin->setSingleStep(.01);
                mXSpin->setDecimals(3);

                // The Ruler range is much wider based on the minimal zoom
                const double tCenter = (mResultCurrentMinX + mResultCurrentMaxX) / 2.;
                const double tSpan = mResultCurrentMaxX - mResultCurrentMinX;
                const double tRangeMin = tCenter - ((tSpan/2.) / sliderToZoom(mXSlider->minimum()));
                const double tRangeMax = tCenter + ((tSpan/2.) / sliderToZoom(mXSlider->minimum()));

                mRuler->setRange(tRangeMin, tRangeMax);
                mRuler->setFormatFunctX(nullptr);

    } else if ( mCurrentTypeGraph == GraphViewResults::eTrace ||
                mCurrentTypeGraph == GraphViewResults::eAccept) {
        int idSelect = 0;
        for (auto && chRadio : mChainRadios) {
            if (chRadio->isChecked())
                break;
            ++idSelect;
        }

        const ChainSpecs& chain = mModel->mChains.at(idSelect);
        const int adaptSize = chain.mBatchIndex * chain.mIterPerBatch;
        const int runSize = chain.mRealyAccepted;
        mResultMaxX = 1 +  chain.mIterPerBurn + adaptSize + runSize;
        // The min is always 0
        mResultMinX = 0.;

        mRuler->addArea(0., 1+ chain.mIterPerBurn, QColor(235, 115, 100));
        mRuler->addArea(1+ chain.mIterPerBurn, 1+chain.mIterPerBurn + adaptSize, QColor(250, 180, 90));
        mRuler->addArea(1+ chain.mIterPerBurn + adaptSize, mResultMaxX, QColor(130, 205, 110));
        // The Ruler range is set exactly to the min and max (impossible to scroll outside)
        mRuler->setRange(mResultMinX, mResultMaxX);
        mRuler->setFormatFunctX(nullptr);


        // The zoom slider and spin are linear.
        // The number of zoom levels depends on the number of iterations (by a factor 100)
        // e.g. 400 iterations => 4 levels
        const int zoomLevels = (int) mResultMaxX / 100;
        mXSlider->setRange(1, zoomLevels);
        mXSpin->setRange(1, zoomLevels);
        mXSpin->setSingleStep(1.);
        mXSpin->setDecimals(0);


    } else if ( mCurrentTypeGraph == GraphViewResults::eCorrel) {
        // The x axis represents h, always in [0, 40]
        mResultMinX = 0.;
        mResultMaxX = 40.;

        // The zoom slider and spin are linear.
        // Always 5 zoom levels
        mXSlider->setRange(1, 5);
        mXSpin->setRange(1, 5);
        mXSpin->setSingleStep(1.);
        mXSpin->setDecimals(0);

        mRuler->setRange(mResultMinX, mResultMaxX);
        mRuler->setFormatFunctX(nullptr);
    }


    // ------------------------------------------------------------------
    //  Apply to Ruler
    // ------------------------------------------------------------------
    mRuler->setCurrent(mResultCurrentMinX, mResultCurrentMaxX);
    mRuler->setScaleDivision(mMajorScale, mMinorCountScale);

    // -------------------------------------------------------
    //  Apply to all graphs
    // -------------------------------------------------------
    QList<GraphViewResults*> graphs = currentGraphs(false);
    for (GraphViewResults*& graph : graphs) {
        graph->setRange(mRuler->mMin, mRuler->mMax);
        graph->setCurrentX(mResultCurrentMinX, mResultCurrentMaxX);
        graph->changeXScaleDivision(mMajorScale, mMinorCountScale);
        graph->zoom(mResultCurrentMinX, mResultCurrentMaxX);
    }

    // -------------------------------------------------------
    //  Set options UI components values
    // -------------------------------------------------------
    setXRange();
    setXSlider(zoomToSlider(mResultZoomX));
    setXSpin(mResultZoomX);
    setXScale();


}

void ResultsView::updateOptionsWidget()
{
    bool isPostDistrib = isPostDistribGraph();
    unsigned optionWidgetHeigth = 0;
    // -------------------------------------------------------------------------------------
    //  Update graph list tab
    // -------------------------------------------------------------------------------------
    if (isChronocurve()) {
        mGraphListTab->setTabVisible(1, false); // Phases
        mGraphListTab->setTabVisible(2, false); // Tempo
        mGraphListTab->setTabVisible(3, true); // Curve
        
        // If the current tab index is "Phases" or "Tempo",
        // then go to "Events" tab which is a good default choice in "Curve" mode
        if (mGraphListTab->currentIndex() == 1 || mGraphListTab->currentIndex() == 2) {
            mGraphListTab->setTab(0, false);
        }
    } else {
        mGraphListTab->setTabVisible(1, mHasPhases); // Phases
        mGraphListTab->setTabVisible(2, mHasPhases); // Tempo
        mGraphListTab->setTabVisible(3, mHasPhases); // Curve
        
        // If the current tab is not currently visible :
        // - Show the "Phases" tab (1) which is a good default choice if the model has phases.
        // - Show the "Events" tab (0) which is a good default choice if the model doesn't have phases.
        
        if (mHasPhases && mGraphListTab->currentIndex() >= 3) {
            mGraphListTab->setTab(1, false);

        } else if (!mHasPhases && mGraphListTab->currentIndex() >= 1) {
            mGraphListTab->setTab(1, false);
        }
    }

    optionWidgetHeigth += mGraphListTab->height();
    // -------------------------------------------------------------------------------------
    //  Update controls depending on current graph list
    // -------------------------------------------------------------------------------------
    if (mGraphListTab->currentName() == tr("Events")) {
        mGraphTypeTabs->setTabVisible(1, true); // History Plot
        mGraphTypeTabs->setTabVisible(2, true); // Acceptance Rate
        mGraphTypeTabs->setTabVisible(3, true); // Autocorrelation

        mEventsPhasesGroup->setVisible(true);
        mTempoGroup->setVisible(false);
        mCurvesGroup->setVisible(false);

        mEventsfoldCheck->setVisible(false);

        if (isChronocurve()) {
            mDataVGRadio->setVisible(true);

        } else {
            mDataVGRadio->setVisible(false);
        }
        
        if (mCurrentVariable == GraphViewResults::eVG)  {
            mDatesfoldCheck->setChecked(false);
            mDatesfoldCheck->setVisible(false);
            mDataCalibCheck->setVisible(false);
            mWiggleCheck->setVisible(false);

        } else {
            mDatesfoldCheck->setVisible(true);
            
            bool showCalibControl = isPostDistrib && mDataThetaRadio->isChecked() && mDatesfoldCheck->isChecked();
            
            mDataCalibCheck->setVisible(showCalibControl);
            mWiggleCheck->setVisible(showCalibControl);
        }

        optionWidgetHeigth += mEventsPhasesGroup->height();
            
    } else if (mGraphListTab->currentIndex() == 1) { // phases tab
        mGraphTypeTabs->setTabVisible(1, true); // History Plot
        mGraphTypeTabs->setTabVisible(2, true); // Acceptance Rate
        mGraphTypeTabs->setTabVisible(3, true); // Autocorrelation

        mEventsPhasesGroup->setVisible(true);
        mTempoGroup->setVisible(false);
        mCurvesGroup->setVisible(false);
        optionWidgetHeigth += mEventsPhasesGroup->height();

        mEventsfoldCheck->setVisible(true);
        optionWidgetHeigth += mEventsfoldCheck->height();

        mDatesfoldCheck->setVisible(mEventsfoldCheck->isChecked());
        if (mDatesfoldCheck->isVisible())
            optionWidgetHeigth += mDatesfoldCheck->height();

        if (!mEventsfoldCheck->isChecked()) {
            mDatesfoldCheck->setChecked(false);
        }

        bool showCalibControl = isPostDistrib && mEventsfoldCheck->isChecked() && mDatesfoldCheck->isChecked() && mDataThetaRadio->isChecked();

        mDataCalibCheck->setVisible(showCalibControl);
        mWiggleCheck->setVisible(showCalibControl);
        if (mDataCalibCheck->isVisible())
            optionWidgetHeigth += mDataCalibCheck->height() + mWiggleCheck->height();

    } else if(mGraphListTab->currentIndex() == 2) { // phases Tempo
        mGraphTypeTabs->setTabVisible(1, false); // History Plot
        mGraphTypeTabs->setTabVisible(2, false); // Acceptance Rate
        mGraphTypeTabs->setTabVisible(3, false); // Autocorrelation
        mGraphTypeTabs->setTab(0, true);

        mEventsPhasesGroup->setVisible(false);
        mTempoGroup->setVisible(true);
        mCurvesGroup->setVisible(false);
        optionWidgetHeigth += mTempoGroup->height();

        mTempoCredCheck->setVisible(mTempoRadio->isChecked());
        mTempoErrCheck->setVisible(mTempoRadio->isChecked());
        if (mTempoCredCheck->isVisible())
            optionWidgetHeigth += mTempoCredCheck->height() + mTempoErrCheck->height();


    } else if (mGraphListTab->currentName() == tr("Curve")) { // curve Tab
        const bool isLambda = mLambdaRadio->isChecked();
        
        mGraphTypeTabs->setTabVisible(1, isLambda); // History Plot
        mGraphTypeTabs->setTabVisible(2, isLambda); // Acceptance Rate
        mGraphTypeTabs->setTabVisible(3, isLambda); // Autocorrelation
        
        mEventsPhasesGroup->setVisible(false);
        mTempoGroup->setVisible(false);
        mCurvesGroup->setVisible(true);
        const int h = mCurveErrorCheck->height();
        int totalH = 5 * h *1.5; // look ligne 165 in ResultsView() comment Right Part. totalH = 3 * h
        if (mCurveGRadio->isChecked()) {
            mCurveErrorCheck->setVisible(true);
            mCurveEventsPointsCheck->setVisible(true);
            mCurveDataPointsCheck->setVisible(true);
            totalH += 3 * h *1.5;

        } else {
            mCurveErrorCheck->setVisible(false);
            mCurveEventsPointsCheck->setVisible(false);
            mCurveDataPointsCheck->setVisible(false);
        }


        mCurvesGroup->setFixedHeight(totalH);
        optionWidgetHeigth += mCurvesGroup->height();
    }




    optionWidgetHeigth += mDensityOptsTitle->height();
    optionWidgetHeigth += mDensityOptsGroup->height();
    // ------------------------------------
    //  Display / Distrib. Option
    // ------------------------------------
   optionWidgetHeigth += mDisplayDistribTab->height();

    toggleDisplayDistrib();
    if (mDisplayDistribTab->currentName() == tr("Display") )
        optionWidgetHeigth += mDisplayWidget->height();

    else
        optionWidgetHeigth += mDistribWidget->height();

    // -------------------------------------------------------------------------------------
    //  Page / Save
    // -------------------------------------------------------------------------------------
    optionWidgetHeigth += mPageSaveTab->height();

    togglePageSave();
    if (mPageSaveTab->currentName() == tr("Page") )
        optionWidgetHeigth += mPageWidget->height();

    else  if (currentGraphs(true).isEmpty())
        optionWidgetHeigth += mSaveAllWidget->height();

    else
        optionWidgetHeigth += mSaveSelectWidget->height();

    mOptionsWidget->setGeometry(0, 0, mOptionsW - mSbe, optionWidgetHeigth);
}


#pragma mark Utilities

bool ResultsView::isPostDistribGraph()
{
    return (mCurrentTypeGraph == GraphViewResults::ePostDistrib);
}

bool ResultsView::xScaleRepresentsTime()
{
    return isPostDistribGraph() && ( mCurrentVariable == GraphViewResults::eTheta ||
                                     mCurrentVariable == GraphViewResults::eTempo ||
                                     mCurrentVariable == GraphViewResults::eActivity ||
                                     mCurrentVariable == GraphViewResults::eG ||
                                     mCurrentVariable == GraphViewResults::eGP ||
                                     mCurrentVariable == GraphViewResults::eGS );
}

double ResultsView::sliderToZoom(const int &coef)
{
    return isPostDistribGraph() ? pow(10., double (coef/100.)) : coef;
}

int ResultsView::zoomToSlider(const double &zoom)
{
    return isPostDistribGraph() ? int (ceil(log10(zoom) * (100.))) : int(zoom);
}

void ResultsView::updateGraphsHeight()
{
    const double min = 2 * AppSettings::heigthUnit();
    const double origin = GraphViewResults::mHeightForVisibleAxis;
    const double prop = mYSpin->value() / 100.;
    mGraphHeight = min + prop * (origin - min);
    updateGraphsLayout();
}

void ResultsView::updateZoomX()
{
    // Pick the value from th spin or the slider
    double zoom = mXSpin->value();

    mResultZoomX = 1./zoom;

    const double tCenter = (mResultCurrentMaxX + mResultCurrentMinX)/2.;
    const double span = (mResultMaxX - mResultMinX)* (1./ zoom);

    double curMin = tCenter - span/2.;
    double curMax = tCenter + span/2.;

    if (curMin < mRuler->mMin) {
        curMin = mRuler->mMin;
        curMax = curMin + span;

    } else if (curMax > mRuler->mMax) {
        curMax = mRuler->mMax;
        curMin = curMax - span;
    }

    mResultCurrentMinX = curMin;
    mResultCurrentMaxX = curMax;

    mRuler->setCurrent(mResultCurrentMinX, mResultCurrentMaxX);

    setXRange();

    updateGraphsZoomX();
}

void ResultsView::updateGraphsZoomX()
{
    // ------------------------------------------------------
    //  Update graphs zoom and scale
    // ------------------------------------------------------
    QList<GraphViewResults*> graphs = currentGraphs(false);
    for (GraphViewResults*& graph : graphs) {
        graph->changeXScaleDivision(mMajorScale, mMinorCountScale);
        graph->zoom(mResultCurrentMinX, mResultCurrentMaxX);
    }

    // ------------------------------------------------------
    //  Store zoom and scale for this type of graph
    // ------------------------------------------------------
    QPair<GraphViewResults::Variable, GraphViewResults::TypeGraph> key(mCurrentVariable, mCurrentTypeGraph);

    if (xScaleRepresentsTime()) {
        double minFormatted = DateUtils::convertFromAppSettingsFormat(mResultCurrentMinX);
        double maxFormatted = DateUtils::convertFromAppSettingsFormat(mResultCurrentMaxX);

        std::pair<double, double> resultMinMax = std::minmax(minFormatted, maxFormatted);

        mZooms[key] = QPair<double, double>(resultMinMax.first, resultMinMax.second);

    } else {
        mZooms[key] = QPair<double, double>(mResultCurrentMinX, mResultCurrentMaxX);
    }

    mScales[key] = QPair<double, int>(mMajorScale, mMinorCountScale);
}

#pragma mark Controls setters

void ResultsView::setXRange()
{
    QLocale locale = QLocale();

    mCurrentXMinEdit->blockSignals(true);
    mCurrentXMaxEdit->blockSignals(true);

    mCurrentXMinEdit->setText(locale.toString(mResultCurrentMinX,'f',0));
    mCurrentXMaxEdit->setText(locale.toString(mResultCurrentMaxX,'f',0));

    mCurrentXMinEdit->blockSignals(false);
    mCurrentXMaxEdit->blockSignals(false);
}

void ResultsView::setXSpin(const double value)
{
    mXSpin->blockSignals(true);
    mXSpin->setValue(value);
    mXSpin->blockSignals(false);
}

void ResultsView::setXSlider(const int value)
{
    mXSlider->blockSignals(true);
    mXSlider->setValue(value);
    mXSlider->blockSignals(false);
}

void ResultsView::setXScale()
{
    //mMinorScaleEdit->blockSignals(true);
    //mMajorScaleEdit->blockSignals(true);

    QLocale locale = QLocale();
    mMinorScaleEdit->setText(locale.toString(mMinorCountScale));
    mMajorScaleEdit->setText(locale.toString(mMajorScale));

    //mMinorScaleEdit->blockSignals(false);
    //mMajorScaleEdit->blockSignals(false);
}

#pragma mark Controls actions

void ResultsView::applyRuler(const double min, const double max)
{
    mResultCurrentMinX = min;
    mResultCurrentMaxX = max;

    setXRange();
    updateGraphsZoomX();
}

void ResultsView::applyStudyPeriod()
{
    if (xScaleRepresentsTime()) {
      mResultCurrentMinX = mModel->mSettings.getTminFormated();
      mResultCurrentMaxX = mModel->mSettings.getTmaxFormated();

    } else if ( mCurrentVariable == GraphViewResults::eSigma ||
                    mCurrentVariable == GraphViewResults::eDuration ||
                    mCurrentVariable == GraphViewResults::eVG  ) {
        mResultCurrentMinX = 0.;
        mResultCurrentMaxX = mResultMaxX;

    } else if ( mCurrentVariable == GraphViewResults::eLambda ) {
        mResultCurrentMinX = mResultMinX;
        mResultCurrentMaxX = mResultMaxX;

    } else if (mCurrentTypeGraph == GraphViewResults::eCorrel) {
        mResultCurrentMinX = 0;
        mResultCurrentMaxX = 40;

    } else if ( mCurrentTypeGraph == GraphViewResults::eTrace ||
                mCurrentTypeGraph == GraphViewResults::eAccept) {
        int idSelect = 0;
        for (auto && chRadio : mChainRadios) {
               if (chRadio->isChecked())
                      break;
                   ++idSelect;
        }
        const ChainSpecs& chain = mModel->mChains.at(idSelect);
        const int adaptSize = chain.mBatchIndex * chain.mIterPerBatch;
        const int runSize = chain.mRealyAccepted;
        // The min is always 0
        mResultCurrentMinX = 0.;
        mResultCurrentMaxX = 1 +  chain.mIterPerBurn + adaptSize + runSize;

    }
        
    mResultZoomX = (mResultMaxX - mResultMinX)/(mResultCurrentMaxX - mResultCurrentMinX);

    Scale Xscale;
    Xscale.findOptimal(mResultCurrentMinX, mResultCurrentMaxX, 10);

    mMajorScale = Xscale.mark;
    mMinorCountScale = Xscale.tip;

    mRuler->setScaleDivision(Xscale);
    mRuler->setCurrent(mResultCurrentMinX, mResultCurrentMaxX);

    updateGraphsZoomX();

    setXRange();
    setXSlider(zoomToSlider(mResultZoomX));
    setXSpin(mResultZoomX);
    setXScale();

}

void ResultsView::applyXRange()
{
    // --------------------------------------------------
    //  Find new current min & max (check range validity !)
    //  Update mResultZoomX
    // --------------------------------------------------
    QString minStr = mCurrentXMinEdit->text();
    bool minIsNumber = true;
    double min = locale().toDouble(minStr, &minIsNumber);

    QString maxStr = mCurrentXMaxEdit->text();
    bool maxIsNumber = true;
    double max = locale().toDouble(maxStr, &maxIsNumber);

    if (minIsNumber && maxIsNumber && ((min != mResultCurrentMinX) || (max != mResultCurrentMaxX))) {
        mResultCurrentMinX = std::max(min, mRuler->mMin);
        mResultCurrentMaxX = std::min(max, mRuler->mMax);

        mResultZoomX = (mResultMaxX - mResultMinX)/ (mResultCurrentMaxX - mResultCurrentMinX);

        mRuler->setCurrent(mResultCurrentMinX, mResultCurrentMaxX);

        updateGraphsZoomX();

        setXRange();
        setXSlider(zoomToSlider(mResultZoomX));
        setXSpin(mResultZoomX);
    }
}

void ResultsView::applyXSlider(int value)
{
    setXSpin(sliderToZoom(value));
    updateZoomX();
}

void ResultsView::applyXSpin(double value)
{
    setXSlider(zoomToSlider(value));
    updateZoomX();
}

void ResultsView::applyXScale()
{
    QString majorStr = mMajorScaleEdit->text();
    bool isMajorNumber = true;
    double majorNumber = locale().toDouble(majorStr, &isMajorNumber);
    if (!isMajorNumber || majorNumber < 1)
        return;

    QString minorStr = mMinorScaleEdit->text();
    bool isMinorNumber = true;
    double minorNumber = locale().toDouble(minorStr, &isMinorNumber);
    if (!isMinorNumber || minorNumber <= 1)
        return;

    mMajorScale = majorNumber;
    mMinorCountScale = (int) minorNumber;

    mRuler->setScaleDivision(mMajorScale, mMinorCountScale);

    QList<GraphViewResults*> graphs = currentGraphs(false);
    for (GraphViewResults*& graph : graphs) {
        graph->changeXScaleDivision(mMajorScale, mMinorCountScale);
    }

    QPair<GraphViewResults::Variable, GraphViewResults::TypeGraph> key(mCurrentVariable, mCurrentTypeGraph);
    mScales[key] = QPair<double, int>(mMajorScale, mMinorCountScale);
}

void ResultsView::applyYSlider(int value)
{
    mYSpin->blockSignals(true);
    mYSpin->setValue(value);
    mYSpin->blockSignals(false);

    updateGraphsHeight();
}

void ResultsView::applyYSpin(int value)
{
    mYSlider->blockSignals(true);
    mYSlider->setValue(value);
    mYSlider->blockSignals(false);

    updateGraphsHeight();
}

void ResultsView::applyFont()
{
    bool ok = false;
    const QFont& currentFont =  mFontBut->font();
    QFont font(QFontDialog::getFont(&ok, currentFont, this));
    if (ok) {
        QList<GraphViewResults*> graphs = allGraphs();
        for (GraphViewResults*& graph : graphs) {
            graph->setGraphsFont(font);
        }

        mFontBut->setText(font.family() + ", " + QString::number(font.pointSizeF()));
    }
}

void ResultsView::applyThickness(int value)
{
    QList<GraphViewResults*> graphs = allGraphs();
    for (GraphViewResults*& graph : graphs) {
        graph->setGraphsThickness(value);
    }
}

void ResultsView::applyOpacity(int value)
{
    const int opValue = value * 10;
    QList<GraphViewResults*> graphs = allGraphs();
    for (GraphViewResults*& graph : graphs) {
        graph->setGraphsOpacity(opValue);
    }
}

void ResultsView::applyFFTLength()
{
    const int len = mFFTLenCombo->currentText().toInt();
    mModel->setFFTLength(len);
}

void ResultsView::applyBandwidth()
{
    const double bandwidth = mBandwidthSpin->value();
    mModel->setBandwidth(bandwidth);
}

void ResultsView::applyThreshold()
{
    const double hpd = locale().toDouble(mThresholdEdit->text());
    mModel->setThreshold(hpd);
}

void ResultsView::applyNextPage()
{
    if ((mCurrentPage + 1) * mGraphsPerPage < mMaximunNumberOfVisibleGraph) {
        ++mCurrentPage;
        updateOptionsWidget();
        createGraphs();
        updateLayout();
    }
}

void ResultsView::applyPreviousPage()
{
    if (mCurrentPage > 0) {
        --mCurrentPage;
        updateOptionsWidget();
        createGraphs();
        updateLayout();
    }
}

void ResultsView::applyGraphsPerPage(int graphsPerPage)
{
    mGraphsPerPage = graphsPerPage;
    mCurrentPage = 0;
    updateOptionsWidget();
    createGraphs();
    updateLayout();
}

void ResultsView::showInfos(bool show)
{
    mTempoStatCheck->setChecked(show);
    mStatCheck->setChecked(show);
    mCurveStatCheck->setChecked(show);
    
    QList<GraphViewResults*> graphs = allGraphs();
    for (GraphViewResults*& graph : graphs) {
        graph->showNumericalResults(show);
    }
    updateLayout();
}

#pragma mark Graph selection and export

void ResultsView::togglePageSave()
{
    // Search for the visible widget
    QWidget* widFrom = nullptr;
    if (mSaveSelectWidget->isVisible())
        widFrom = mSaveSelectWidget;

    else if (mSaveAllWidget->isVisible())
        widFrom = mSaveAllWidget;

    else if (mPageWidget->isVisible())
        widFrom = mPageWidget;

    // Exchange with the widget corresponding to the requested tab
    if (mPageSaveTab->currentName() == tr("Page") ) { // Tab = Page
        // -------------------------------------------------------------------------------------
        //  - Update the total number of graphs for all pages
        //  - Check if the current page is still lower than the number of pages
        //  - Update the pagination display
        //  => All this must be done BEFORE calling createGraphs, which uses theses params to build the graphs
        // -------------------------------------------------------------------------------------
        updateTotalGraphs();

        int numPages = ceil((double)mMaximunNumberOfVisibleGraph / (double)mGraphsPerPage);
        if (mCurrentPage >= numPages) {
            mCurrentPage = 0;
        }

        mPageEdit->setText(locale().toString(mCurrentPage + 1) + "/" + locale().toString(numPages));

        mPageWidget->setVisible(true);
        mSaveSelectWidget->setVisible(false);
        mSaveAllWidget->setVisible(false);

        if (widFrom != mPageWidget)
            mOptionsLayout->replaceWidget(widFrom, mPageWidget);

    } else  { // Tab = Saving
        mPageWidget->setVisible(false);

        const bool hasSelection = (currentGraphs(true).size() > 0);
        mSaveSelectWidget->setVisible(hasSelection);
        mSaveAllWidget->setVisible(!hasSelection);

        if (hasSelection && widFrom != mSaveSelectWidget) {
               mOptionsLayout->replaceWidget(widFrom, mSaveSelectWidget);

        } else if (widFrom != mSaveAllWidget) {
              mOptionsLayout->replaceWidget(widFrom, mSaveAllWidget);

        }

    }


}

void ResultsView::saveGraphData()
{
    QList<GraphViewResults*> graphs = currentGraphs(true);
    for (auto&& graph : graphs) {
        graph->saveGraphData();
    }
}

void ResultsView::resultsToClipboard()
{
    QString resultText;
    QList<GraphViewResults*> graphs = currentGraphs(true);
    for (auto&& graph : graphs) {
        resultText += graph->getTextAreaToPlainText();
    }
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(resultText);
}

void ResultsView::imageToClipboard()
{
    QList<GraphViewResults*> graphs = currentGraphs(true);

    if (!graphs.isEmpty()) {
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
        for (auto&& graph : graphs) {
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
    if (graphs.isEmpty()) {
        return;
    }

    // --------------------------------------------------
    //  Ask for a file name and type (SVG or Image)
    // --------------------------------------------------
    QString fileName = QFileDialog::getSaveFileName(qApp->activeWindow(),
        tr("Save graph image as..."),
        MainWindow::getInstance()->getCurrentPath(),
        QObject::tr("Image (*.png);;Photo (*.jpg);;Scalable Vector Graphics (*.svg)"));

    if (!fileName.isEmpty()) {
        // --------------------------------------------------
        //  Get the file extension
        // --------------------------------------------------
        QFileInfo fileInfo;
        fileInfo = QFileInfo(fileName);
        QString fileExtension = fileInfo.suffix();
        bool asSvg = fileName.endsWith(".svg");


        //const int heightText (2 * qApp->fontMetrics().height());
        QFontMetricsF fm (qApp->font());
        const int heightText (2 * fm.height());
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
            for (auto&& graph : graphs) {
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

            } else if (fileExtension == "jpg") {
                int imageQuality = AppSettings::mImageQuality;
                image.save(fileName, "jpg",imageQuality);

            } else if (fileExtension == "bmp") {
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
            for (auto&& graph : graphs) {
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
            if (dir.exists()) {
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
                output<<"<!DOCTYPE html>"<< Qt::endl;
                output<<"<html>"<< Qt::endl;
                output<<"<body>"<< Qt::endl;

                output<<"<h2>"<< version << "</h2>" << Qt::endl;
                output<<"<h2>"<< projectName+ "</h2>" << Qt::endl;
                output<<"<hr>";
                output<<mModel->getModelLog();

                output<<"</body>"<< Qt::endl;
                output<<"</html>"<< Qt::endl;
            }
            file.close();

            file.setFileName(dirPath + "/Log_MCMC_Initialization.html");

            if (file.open(QFile::WriteOnly | QFile::Truncate)) {
                QTextStream output(&file);
                output<<"<!DOCTYPE html>"<< Qt::endl;
                output<<"<html>"<< Qt::endl;
                output<<"<body>"<< Qt::endl;

                output<<"<h2>"<< version << "</h2>" << Qt::endl;
                output<<"<h2>"<< projectName+ "</h2>" << Qt::endl;
                output<<"<hr>";
                output<<mModel->getInitLog();

                output<<"</body>"<< Qt::endl;
                output<<"</html>"<< Qt::endl;
            }
            file.close();

            file.setFileName(dirPath + "/Log_MCMC_Adaptation.html");

            if (file.open(QFile::WriteOnly | QFile::Truncate)) {
                QTextStream output(&file);
                output<<"<!DOCTYPE html>"<< Qt::endl;
                output<<"<html>"<< Qt::endl;
                output<<"<body>"<< Qt::endl;

                output<<"<h2>"<< version << "</h2>" << Qt::endl;
                output<<"<h2>"<< projectName+ "</h2>" << Qt::endl;
                output<<"<hr>";
                output<<mModel->getAdaptLog();

                output<<"</body>"<< Qt::endl;
                output<<"</html>"<< Qt::endl;
            }
            file.close();

            file.setFileName(dirPath + "/Log_Posterior_Distrib_Stats.html");

            if (file.open(QFile::WriteOnly | QFile::Truncate)) {
                QTextStream output(&file);
                output<<"<!DOCTYPE html>"<< Qt::endl;
                output<<"<html>"<< Qt::endl;
                output<<"<body>"<< Qt::endl;

                output<<"<h2>"<< version << "</h2>" << Qt::endl;
                output<<"<h2>"<< projectName+ "</h2>" << Qt::endl;
                output<<"<hr>";
                output<<mModel->getResultsLog();

                output<<"</body>"<< Qt::endl;
                output<<"</html>"<< Qt::endl;
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
    bool printAxis = (mGraphHeight < GraphViewResults::mHeightForVisibleAxis);

    QWidget* curWid (nullptr);

    if (mGraphListTab->currentIndex() == 0) {
        curWid = mEventsScrollArea->widget();
        curWid->setFont(mByEventsGraphs.at(0)->font());

    } else if (mGraphListTab->currentIndex() == 1) {
        curWid = mPhasesScrollArea->widget();
        curWid->setFont(mByPhasesGraphs.at(0)->font());

    } else if (mGraphListTab->currentIndex() == 2) {
        curWid = mTempoScrollArea->widget();
        curWid->setFont(mByTempoGraphs.at(0)->font());

    } else if (mGraphListTab->currentIndex() == 3) {
        curWid = mCurveScrollArea->widget();
        curWid->setFont(mByCurveGraphs.at(0)->font());


    } else
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
        if (mCurrentTypeGraph == GraphViewResults::ePostDistrib && ( mCurrentVariable == GraphViewResults::eTheta ||
                                                                     mCurrentVariable == GraphViewResults::eActivity||
                                                                     mCurrentVariable == GraphViewResults::eG ||
                                                                     mCurrentVariable == GraphViewResults::eGP ||
                                                                     mCurrentVariable == GraphViewResults::eGS) )
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

#pragma mark Chronocurve

bool ResultsView::isChronocurve() const
{
    if (mModel)
        return mModel->mProject->isChronocurve();
    else
        return false;
}

ModelChronocurve* ResultsView::modelChronocurve() const
{
    return dynamic_cast<ModelChronocurve*>(mModel);
}
