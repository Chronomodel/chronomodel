/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2023

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
#include "GraphViewLambda.h"
#include "GraphViewCurve.h"
#include "GraphViewS02.h"

#include "Tabs.h"
#include "Ruler.h"
#include "Marker.h"

#include "Date.h"
#include "Event.h"
#include "Bound.h"
#include "Phase.h"

#include "Label.h"
#include "Button.h"
#include "LineEdit.h"
#include "CheckBox.h"
#include "RadioButton.h"

#include "MainWindow.h"
#include "Project.h"

#include "QtUtilities.h"
#include "StdUtilities.h"
#include "ModelUtilities.h"
#include "DoubleValidator.h"

#include "AppSettings.h"

#include "ModelCurve.h"

#include <iostream>

#include <QtWidgets>
#include <QtSvg>
#include <QFontDialog>


ResultsView::ResultsView(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
    mModel(nullptr),

    mMargin(5),
    mOptionsW(250),
    mMarginLeft(40),
    mMarginRight(40),

    mCurrentTypeGraph(GraphViewResults::ePostDistrib),
    mCurrentVariableList(GraphViewResults::eThetaEvent),
    mMainVariable(GraphViewResults::eThetaEvent),
    mHasPhases(false),
    mResultZoomT(1.),
    mResultMinT(0.),
    mResultMaxT(0.),
    mResultCurrentMinT(0.),
    mResultCurrentMaxT(0.),

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
    mEventsScrollArea->QScrollArea::setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mEventsScrollArea->setMouseTracking(true);
    QWidget* eventsWidget = new QWidget();
    eventsWidget->setMouseTracking(true);
    mEventsScrollArea->setWidget(eventsWidget);

    mPhasesScrollArea = new QScrollArea(this);
    mPhasesScrollArea->QScrollArea::setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mPhasesScrollArea->setMouseTracking(true);
    QWidget* phasesWidget = new QWidget();
    phasesWidget->setMouseTracking(true);
    mPhasesScrollArea->setWidget(phasesWidget);

    mCurveScrollArea = new QScrollArea(this);
    mCurveScrollArea->QScrollArea::setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

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
    mEventsGroup = new QWidget();

    mEventsDatesUnfoldCheck = new CheckBox(tr("Unfold Data"));
    mEventsDatesUnfoldCheck->setFixedHeight(h);
    mEventsDatesUnfoldCheck->setToolTip(tr("Display Events' data"));

    mEventThetaRadio = new RadioButton(tr("Event Date"));
    mEventThetaRadio->setFixedHeight(h);
    mEventThetaRadio->setChecked(true);

    mDataSigmaRadio = new RadioButton(tr("Std ti"));
    mDataSigmaRadio->setFixedHeight(h);
    
    mEventVGRadio = new RadioButton(tr("Std gi"));
    mEventVGRadio->setFixedHeight(h);

    mDataCalibCheck = new CheckBox(tr("Individual Calib. Dates"));
    mDataCalibCheck->setFixedHeight(h);
    mDataCalibCheck->setChecked(true);

    mWiggleCheck = new CheckBox(tr("Wiggle shifted"));
    mWiggleCheck->setFixedHeight(h);

    mStatCheck = new CheckBox(tr("Show Stat."));
    mStatCheck->setFixedHeight(h);
    mStatCheck->setToolTip(tr("Display numerical results computed on posterior densities below all graphs."));

    QVBoxLayout* resultsGroupLayout = new QVBoxLayout();
    resultsGroupLayout->setContentsMargins(10, 10, 10, 10);
    resultsGroupLayout->setSpacing(15);
    resultsGroupLayout->addWidget(mEventThetaRadio);
    resultsGroupLayout->addWidget(mDataSigmaRadio);
    resultsGroupLayout->addWidget(mEventVGRadio);

    resultsGroupLayout->addWidget(mEventsDatesUnfoldCheck);
    resultsGroupLayout->addWidget(mDataCalibCheck);
    resultsGroupLayout->addWidget(mWiggleCheck);
    resultsGroupLayout->addWidget(mStatCheck);

    mEventsGroup->resize(8 * h, mOptionsW);
    mEventsGroup->setLayout(resultsGroupLayout);


    // -----------------------------------------------------------------
    //  Tempo Group (if graph list tab = duration)
    // -----------------------------------------------------------------
    mPhasesGroup = new QWidget();
    mBeginEndRadio = new RadioButton(tr("Begin-End"), mPhasesGroup);
    mBeginEndRadio->setFixedHeight(h);
    mBeginEndRadio->setChecked(true);

    mPhasesEventsUnfoldCheck = new CheckBox(tr("Unfold Events"), mPhasesGroup);
    mPhasesEventsUnfoldCheck->setFixedHeight(h);
    mPhasesEventsUnfoldCheck->setToolTip(tr("Display Phases' Pvents"));

    mPhasesDatesUnfoldCheck = new CheckBox(tr("Unfold Data"), mPhasesGroup);
    mPhasesDatesUnfoldCheck->setFixedHeight(h);
    mPhasesDatesUnfoldCheck->setToolTip(tr("Display Events' Pata"));

    mTempoRadio = new RadioButton(tr("Tempo"), mPhasesGroup);
    mTempoRadio->setFixedHeight(h);

    mActivityRadio = new RadioButton(tr("Activity"), mPhasesGroup);
    mActivityRadio->setFixedHeight(h);

    mActivityUnifCheck = new CheckBox(tr("Unif Theo"), mPhasesGroup);
    mActivityUnifCheck->setFixedHeight(h);

    mErrCheck = new CheckBox(tr("Error"), mPhasesGroup);
    mErrCheck->setFixedHeight(h);

    mDurationRadio = new RadioButton(tr("Duration"), mPhasesGroup);
    mDurationRadio->setFixedHeight(h);
    mDurationRadio->setChecked(false);

    mPhasesStatCheck = new CheckBox(tr("Show Stat."));
    mPhasesStatCheck->setFixedHeight(h);
    mPhasesStatCheck->setToolTip(tr("Display numerical results computed on posterior densities below all graphs."));

    QVBoxLayout* phasesGroupLayout = new QVBoxLayout(mPhasesGroup);

    QVBoxLayout* phasesUnfoldGroupLayout = new QVBoxLayout();
    phasesUnfoldGroupLayout->setContentsMargins(15, 0, 0, 0);
    phasesUnfoldGroupLayout->addWidget(mErrCheck, Qt::AlignLeft);
    phasesUnfoldGroupLayout->addWidget(mActivityUnifCheck, Qt::AlignLeft);
    phasesUnfoldGroupLayout->addWidget(mPhasesEventsUnfoldCheck, Qt::AlignLeft);
    phasesUnfoldGroupLayout->addWidget(mPhasesDatesUnfoldCheck, Qt::AlignLeft);

    phasesGroupLayout->setContentsMargins(10, 10, 10, 10);

    phasesGroupLayout->addWidget(mBeginEndRadio);
    phasesGroupLayout->setSpacing(10);
    phasesGroupLayout->addWidget(mTempoRadio);
    phasesGroupLayout->setSpacing(10);
    phasesGroupLayout->addWidget(mActivityRadio);

    phasesGroupLayout->addLayout(phasesUnfoldGroupLayout);

    phasesGroupLayout->addWidget(mDurationRadio);

    phasesGroupLayout->addWidget(mPhasesStatCheck);

    // -----------------------------------------------------------------
    //  Curves Group (if graph list tab = curve)
    // -----------------------------------------------------------------
    mCurvesGroup = new QWidget();
    
    mCurveGRadio = new RadioButton(tr("Mean Curve"), mCurvesGroup);
    mCurveGRadio->setFixedHeight(h);
    mCurveGRadio->setChecked(true);
    
    mCurveErrorCheck = new CheckBox(tr("Error (at 95%)"), mCurvesGroup);
    mCurveErrorCheck->setFixedHeight(h);
    mCurveErrorCheck->setChecked(true);
    
    mCurveMapCheck = new CheckBox(tr("Density Plot"), mCurvesGroup);
    mCurveMapCheck->setFixedHeight(h);
    mCurveMapCheck->setChecked(true);

    mCurveEventsPointsCheck = new CheckBox(tr("Event Dates (HPD)"), mCurvesGroup);
    mCurveEventsPointsCheck->setFixedHeight(h);
    mCurveEventsPointsCheck->setChecked(true);

    mCurveDataPointsCheck = new CheckBox(tr("Ind. Calib. Dates"), mCurvesGroup);
    mCurveDataPointsCheck->setFixedHeight(h);
    mCurveDataPointsCheck->setChecked(true);

    mCurveGPRadio = new RadioButton(tr("Curve Var. Rate"), mCurvesGroup);
    mCurveGPRadio->setFixedHeight(h);
    
    mCurveGSRadio = new RadioButton(tr("Curve Acceleration"), mCurvesGroup);
    mCurveGSRadio->setFixedHeight(h);
    
    mLambdaRadio = new RadioButton(tr("Smoothing"), mCurvesGroup);
    mLambdaRadio->setFixedHeight(h);
    
    mS02VgRadio = new RadioButton(tr("Shrinkage Param."), mCurvesGroup);
    mS02VgRadio->setFixedHeight(h);

    mCurveStatCheck = new CheckBox(tr("Show Stat."));
    mCurveStatCheck->setFixedHeight(h);
    mCurveStatCheck->setToolTip(tr("Display numerical results computed on posterior densities below all graphs."));

    QVBoxLayout* curveGroupLayout = new QVBoxLayout();

    QVBoxLayout* curveOptionGroupLayout = new QVBoxLayout();
    curveOptionGroupLayout->setContentsMargins(15, 0, 0, 0);
    curveOptionGroupLayout->addWidget(mCurveErrorCheck, Qt::AlignLeft);
    curveOptionGroupLayout->addWidget(mCurveMapCheck, Qt::AlignLeft);
    curveOptionGroupLayout->addWidget(mCurveEventsPointsCheck, Qt::AlignLeft);
    curveOptionGroupLayout->addWidget(mCurveDataPointsCheck, Qt::AlignLeft);


    curveGroupLayout->setContentsMargins(10, 10, 10, 10);
    curveGroupLayout->addWidget(mCurveGRadio);
   // curveGroupLayout->setSpacing(10);
    curveGroupLayout->addLayout(curveOptionGroupLayout);
    curveGroupLayout->addWidget(mCurveGPRadio);
    curveGroupLayout->addWidget(mCurveGSRadio);
    curveGroupLayout->addWidget(mLambdaRadio);
    curveGroupLayout->addWidget(mS02VgRadio);
    curveGroupLayout->addWidget(mCurveStatCheck);

    mCurvesGroup->setLayout(curveGroupLayout);

    // -----------------------------------------------------------------
    //  Connections
    // -----------------------------------------------------------------
    connect(mEventThetaRadio, &RadioButton::clicked, this, &ResultsView::applyCurrentVariable);

    connect(mEventsDatesUnfoldCheck, &CheckBox::clicked, this, &ResultsView::applyCurrentVariable);

    connect(mDataSigmaRadio, &RadioButton::clicked, this, &ResultsView::applyCurrentVariable);
    connect(mEventVGRadio, &RadioButton::clicked, this, &ResultsView::applyCurrentVariable);

    connect(mBeginEndRadio, &RadioButton::clicked, this, &ResultsView::applyCurrentVariable);
    connect(mPhasesEventsUnfoldCheck, &CheckBox::clicked, this, &ResultsView::applyCurrentVariable);
    connect(mPhasesDatesUnfoldCheck, &CheckBox::clicked, this, &ResultsView::applyCurrentVariable);

    connect(mTempoRadio, &RadioButton::clicked, this, &ResultsView::applyCurrentVariable);
    connect(mActivityRadio, &RadioButton::clicked, this, &ResultsView::applyCurrentVariable);
    connect(mDurationRadio, &RadioButton::clicked, this, &ResultsView::applyCurrentVariable);

    connect(mDataCalibCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mWiggleCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);

    connect(mStatCheck, &CheckBox::clicked, this, &ResultsView::showStats);

    connect(mErrCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mActivityUnifCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);

    connect(mPhasesStatCheck, &CheckBox::clicked, this, &ResultsView::showStats);
    connect(mCurveStatCheck, &CheckBox::clicked, this, &ResultsView::showStats);
    
    connect(mCurveGRadio, &CheckBox::clicked, this, &ResultsView::applyCurrentVariable);
    connect(mCurveGPRadio, &CheckBox::clicked, this, &ResultsView::applyCurrentVariable);
    connect(mCurveGSRadio, &CheckBox::clicked, this, &ResultsView::applyCurrentVariable);
    connect(mLambdaRadio, &CheckBox::clicked, this, &ResultsView::applyCurrentVariable);
    connect(mS02VgRadio, &CheckBox::clicked, this, &ResultsView::applyCurrentVariable);

    connect(mCurveErrorCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mCurveMapCheck, &CheckBox::clicked, this,  &ResultsView::updateCurvesToShow);
    connect(mCurveEventsPointsCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mCurveDataPointsCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);

    // -----------------------------------------------------------------
    //  Graph List tab (has to be created after mResultsGroup and mTempoGroup)
    // -----------------------------------------------------------------
    mGraphListTab = new Tabs();
    mGraphListTab->setFixedHeight(mGraphListTab->tabHeight());
    mGraphListTab->addTab( tr("Events"));
    mGraphListTab->addTab(tr("Phases"));
    mGraphListTab->addTab(tr("Curves"));

    connect(mGraphListTab, static_cast<void (Tabs::*)(const int&)>(&Tabs::tabClicked), this, &ResultsView::applyGraphListTab);

    // -----------------------------------------------------------------
    //  Tabs : Display / Distrib. Options
    // -----------------------------------------------------------------
    mDisplayDistribTab = new Tabs();
    mDisplayDistribTab->setFixedHeight(mDisplayDistribTab->tabHeight());

    mDisplayDistribTab->addTab(tr("Display"));
    mDisplayDistribTab->addTab(tr("Distrib. Options"));

    // Necessary to reposition all elements inside the selected tab :
    connect(mDisplayDistribTab, static_cast<void (Tabs::*)(const int&)>(&Tabs::tabClicked), this, &ResultsView::updateOptionsWidget);

    // -----------------------------------------------------------------
    //  Display / Span Options
    // -----------------------------------------------------------------
    mDisplayWidget = new QWidget();
    mSpanGroup  = new QWidget();
    h = 20;

    mSpanTitle = new Label(tr("Time Scale"), mDisplayWidget);
    mSpanTitle->setFixedHeight(25);
    mSpanTitle->setIsTitle(true);

    mDisplayStudyBut = new Button(tr("Study Period Display"), mSpanGroup);
    mDisplayStudyBut->setFixedHeight(25);
    mDisplayStudyBut->setToolTip(tr("Restore view with the study period span"));

    mSpanLab = new QLabel(tr("Span"), mSpanGroup);
    mSpanLab->setFixedHeight(h);
    //mSpanLab->setAdjustText(false);

    mCurrentTMinEdit = new LineEdit(mSpanGroup);
    mCurrentTMinEdit->setFixedHeight(h);
    mCurrentTMinEdit->setToolTip(tr("Enter a minimal value to display the curves"));

    mCurrentTMaxEdit = new LineEdit(mSpanGroup);
    mCurrentTMaxEdit->setFixedHeight(h);
    mCurrentTMaxEdit->setToolTip(tr("Enter a maximal value to display the curves"));

    mXLab = new QLabel(tr("time"), mSpanGroup);
    mXLab->setFixedHeight(h);
    mXLab->setAlignment(Qt::AlignCenter);

    mXSlider = new QSlider(Qt::Horizontal, mSpanGroup);
    mXSlider->setFixedHeight(h);
    mXSlider->setRange(-100, 100);
    mXSlider->setTickInterval(1);
    mXSlider->setValue(0);

    mTimeSpin = new QDoubleSpinBox(mSpanGroup);
    mTimeSpin->setFixedHeight(h);
    mTimeSpin->setRange(pow(10., double (mXSlider->minimum()/100.)),pow(10., double (mXSlider->maximum()/100.)));
    mTimeSpin->setSingleStep(.01);
    mTimeSpin->setDecimals(3);
    mTimeSpin->setValue(sliderToZoom(mXSlider->value()));
    mTimeSpin->setToolTip(tr("Enter zoom value to magnify the curves on X span"));

    mMajorScaleLab = new QLabel(tr("Major Interval"), mSpanGroup);
    mMajorScaleLab->setFixedHeight(h);
    mMajorScaleLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mMajorScaleEdit = new LineEdit(mSpanGroup);
    mMajorScaleEdit->setFixedHeight(h);
    mMajorScaleEdit->setText(QString::number(mMajorScale));
    mMajorScaleEdit->setToolTip(tr("Enter a interval for the main division of the axes under the curves, upper than 1"));

    mMinorScaleLab = new QLabel(tr("Minor Interval Count"), mSpanGroup);
    mMinorScaleLab->setFixedHeight(h);
    mMinorScaleLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mMinorScaleEdit = new LineEdit(mSpanGroup);
    mMinorScaleEdit->setFixedHeight(h);
    mMinorScaleEdit->setText(QString::number(mMinorCountScale));
    mMinorScaleEdit->setToolTip(tr("Enter a interval for the subdivision of the Major Interval for the scale under the curves, upper than 1"));

    connect(mDisplayStudyBut, static_cast<void (Button::*)(bool)>(&Button::clicked), this, &ResultsView::applyStudyPeriod);
    connect(mCurrentTMinEdit, &LineEdit::editingFinished, this, &ResultsView::applyTimeRange);
    connect(mCurrentTMaxEdit, &LineEdit::editingFinished, this, &ResultsView::applyTimeRange);
    connect(mXSlider, &QSlider::valueChanged, this, &ResultsView::applyTimeSlider);
    connect(mTimeSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &ResultsView::applyTimeSpin);
    connect(mMajorScaleEdit, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textEdited), this, &ResultsView::applyZoomScale);
    connect(mMinorScaleEdit, static_cast<void (QLineEdit::*)(const QString&)>(&QLineEdit::textEdited), this, &ResultsView::applyZoomScale);

    QHBoxLayout* spanLayout0 = new QHBoxLayout();
    spanLayout0->setContentsMargins(0, 0, 0, 0);
    spanLayout0->addWidget(mDisplayStudyBut);

    QHBoxLayout* spanLayout1 = new QHBoxLayout();
    spanLayout1->setContentsMargins(0, 0, 0, 0);
    spanLayout1->addWidget(mCurrentTMinEdit);
    spanLayout1->addWidget(mSpanLab);
    spanLayout1->addWidget(mCurrentTMaxEdit);

    QHBoxLayout* spanLayout2 = new QHBoxLayout();
    spanLayout2->setContentsMargins(0, 0, 0, 0);
    spanLayout2->addWidget(mXLab);
    spanLayout2->addWidget(mXSlider);
    spanLayout2->addWidget(mTimeSpin);

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
    //  Display / X Options
    // ------------------------------------
    mXOptionGroup  = new QWidget();
    h = 20;

    mXOptionTitle = new Label(tr("X Scale"), mDisplayWidget);
    mXOptionTitle->setFixedHeight(25);
    mXOptionTitle->setIsTitle(true);

    mXOptionBut = new Button(tr("Optimal X Display"), mXOptionGroup);
    mXOptionBut->setFixedHeight(25);
    mXOptionBut->setToolTip(tr("Restore view with the study period span"));

    mXOptionLab = new QLabel(tr("X"), mXOptionGroup);
    mXOptionLab->setFixedHeight(h);
    //mSpanLab->setAdjustText(false);

    mCurrentXMinEdit = new LineEdit(mXOptionGroup);
    mCurrentXMinEdit->setFixedHeight(h);
    mCurrentXMinEdit->setToolTip(tr("Enter a minimal value to display the curves"));

    mCurrentXMaxEdit = new LineEdit(mXOptionGroup);
    mCurrentXMaxEdit->setFixedHeight(h);
    mCurrentXMaxEdit->setToolTip(tr("Enter a maximal value to display the curves"));

    connect(mXOptionBut, static_cast<void (Button::*)(bool)>(&Button::clicked), this, &ResultsView::findOptimalX);
    connect(mCurrentXMinEdit, &LineEdit::editingFinished, this, &ResultsView::applyXRange);
    connect(mCurrentXMaxEdit, &LineEdit::editingFinished, this, &ResultsView::applyXRange);

    QHBoxLayout* XOptionLayout0 = new QHBoxLayout();
    XOptionLayout0->setContentsMargins(5, 0, 5, 0);
    XOptionLayout0->addWidget(mXOptionBut);

    QHBoxLayout* XOptionLayout1 = new QHBoxLayout();
    XOptionLayout1->setContentsMargins(5, 0, 5, 0);
    XOptionLayout1->addWidget(mCurrentXMinEdit);
    XOptionLayout1->addWidget(mXOptionLab);
    XOptionLayout1->addWidget(mCurrentXMaxEdit);

    QVBoxLayout* XOptionLayout = new QVBoxLayout();
    XOptionLayout->setContentsMargins(5, 5, 5, 5);
    XOptionLayout->setSpacing(5);
    XOptionLayout->addLayout(XOptionLayout0);
    XOptionLayout->addLayout(XOptionLayout1);

    mXOptionGroup->setLayout(XOptionLayout);

    // ------------------------------------
    //  Display / Y Options
    // ------------------------------------
    mYOptionGroup  = new QWidget();
    h = 20;

    mYOptionTitle = new Label(tr("Y Scale"), mDisplayWidget);
    mYOptionTitle->setFixedHeight(25);
    mYOptionTitle->setIsTitle(true);

    mYOptionBut = new Button(tr("Optimal Y Display"), mYOptionGroup);
    mYOptionBut->setFixedHeight(25);
    mYOptionBut->setToolTip(tr("Optimize Y scale"));

    mYOptionLab = new QLabel(tr("Y"), mYOptionGroup);
    mYOptionLab->setFixedHeight(h);
    //mSpanLab->setAdjustText(false);

    mCurrentYMinEdit = new LineEdit(mYOptionGroup);
    mCurrentYMinEdit->setFixedHeight(h);
    mCurrentYMinEdit->setToolTip(tr("Enter a minimal value to display the curves"));

    mCurrentYMaxEdit = new LineEdit(mYOptionGroup);
    mCurrentYMaxEdit->setFixedHeight(h);
    mCurrentYMaxEdit->setToolTip(tr("Enter a maximal value to display the curves"));

    connect(mYOptionBut, static_cast<void (Button::*)(bool)>(&Button::clicked), this, &ResultsView::findOptimalY);
    connect(mCurrentYMinEdit, &LineEdit::editingFinished, this, &ResultsView::applyYRange);
    connect(mCurrentYMaxEdit, &LineEdit::editingFinished, this, &ResultsView::applyYRange);

    QHBoxLayout* YOptionLayout0 = new QHBoxLayout();
    YOptionLayout0->setContentsMargins(5, 0, 5, 0);
    YOptionLayout0->addWidget(mYOptionBut);

    QHBoxLayout* YOptionLayout1 = new QHBoxLayout();
    YOptionLayout1->setContentsMargins(5, 0, 5, 0);
    YOptionLayout1->addWidget(mCurrentYMinEdit);
    YOptionLayout1->addWidget(mYOptionLab);
    YOptionLayout1->addWidget(mCurrentYMaxEdit);

    QVBoxLayout* YOptionLayout = new QVBoxLayout();
    YOptionLayout->setContentsMargins(5, 5, 5, 5);
    YOptionLayout->setSpacing(5);
    YOptionLayout->addLayout(YOptionLayout0);
    YOptionLayout->addLayout(YOptionLayout1);

    mYOptionGroup->setLayout(YOptionLayout);

    // ------------------------------------
    //  Display / Z Options
    // ------------------------------------
    mZOptionGroup  = new QWidget();
    h = 20;

    mZOptionTitle = new Label(tr("Z Scale"), mDisplayWidget);
    mZOptionTitle->setFixedHeight(25);
    mZOptionTitle->setIsTitle(true);

    mZOptionBut = new Button(tr("Optimal Z Display"), mZOptionGroup);
    mZOptionBut->setFixedHeight(25);
    mZOptionBut->setToolTip(tr("Optimize Z scale"));

    mZOptionLab = new QLabel(tr("Z"), mZOptionGroup);
    mZOptionLab->setFixedHeight(h);
    //mSpanLab->setAdjustText(false);

    mCurrentZMinEdit = new LineEdit(mZOptionGroup);
    mCurrentZMinEdit->setFixedHeight(h);
    mCurrentZMinEdit->setToolTip(tr("Enter a minimal value to display the curves"));

    mCurrentZMaxEdit = new LineEdit(mZOptionGroup);
    mCurrentZMaxEdit->setFixedHeight(h);
    mCurrentZMaxEdit->setToolTip(tr("Enter a maximal value to display the curves"));

    connect(mZOptionBut, static_cast<void (Button::*)(bool)>(&Button::clicked), this, &ResultsView::findOptimalZ);
    connect(mCurrentZMinEdit, &LineEdit::editingFinished, this, &ResultsView::applyZRange);
    connect(mCurrentZMaxEdit, &LineEdit::editingFinished, this, &ResultsView::applyZRange);

    QHBoxLayout* ZOptionLayout0 = new QHBoxLayout();
    ZOptionLayout0->setContentsMargins(5, 0, 5, 0);
    ZOptionLayout0->addWidget(mZOptionBut);

    QHBoxLayout* ZOptionLayout1 = new QHBoxLayout();
    ZOptionLayout1->setContentsMargins(5, 0, 5, 0);
    ZOptionLayout1->addWidget(mCurrentZMinEdit);
    ZOptionLayout1->addWidget(mZOptionLab);
    ZOptionLayout1->addWidget(mCurrentZMaxEdit);

    QVBoxLayout* ZOptionLayout = new QVBoxLayout();
    ZOptionLayout->setContentsMargins(5, 5, 5, 5);
    XOptionLayout->setSpacing(5);
    ZOptionLayout->addLayout(ZOptionLayout0);
    ZOptionLayout->addLayout(ZOptionLayout1);

    mZOptionGroup->setLayout(ZOptionLayout);

    // ------------------------------------
    //  Display / Graphic Options
    // ------------------------------------
    mGraphicTitle = new Label(tr("Graphic Options"), mDisplayWidget);
    mGraphicTitle->setFixedHeight(25);
    mGraphicTitle->setIsTitle(true);

    mGraphicGroup = new QWidget();

    mZoomLab = new QLabel(tr("Zoom"), mGraphicGroup);
    mZoomLab->setAlignment(Qt::AlignCenter);

    mZoomSlider = new QSlider(Qt::Horizontal, mGraphicGroup);
    mZoomSlider->setRange(10, 1000);
    mZoomSlider->setTickInterval(1);
    mZoomSlider->setValue(100);

    mZoomSpin = new QSpinBox(mGraphicGroup);
    mZoomSpin->setRange(mZoomSlider->minimum(), mZoomSlider->maximum());
    mZoomSpin->setValue(mZoomSlider->value());
    mZoomSpin->setToolTip(tr("Enter zoom value to magnify the curves on Y scale"));

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

    connect(mZoomSlider, &QSlider::valueChanged, this, &ResultsView::applyZoomSlider);
    connect(mZoomSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ResultsView::applyZoomSpin);
    connect(mFontBut, &QPushButton::clicked, this, &ResultsView::applyFont);
    connect(mThicknessCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ResultsView::applyThickness);
    connect(mOpacityCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ResultsView::applyOpacity);

    QHBoxLayout* graphicLayout1 = new QHBoxLayout();
    graphicLayout1->setContentsMargins(0, 0, 0, 0);
    graphicLayout1->addWidget(mZoomLab);
    graphicLayout1->addWidget(mZoomSlider);
    graphicLayout1->addWidget(mZoomSpin);

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

    displayLayout->addWidget(mXOptionTitle);
    displayLayout->addWidget(mXOptionGroup);
    displayLayout->addWidget(mYOptionTitle);
    displayLayout->addWidget(mYOptionGroup);
    displayLayout->addWidget(mZOptionTitle);
    displayLayout->addWidget(mZOptionGroup);

    displayLayout->addWidget(mGraphicTitle);
    displayLayout->addWidget(mGraphicGroup);
    mDisplayWidget->setLayout(displayLayout);

    // ------------------------------------
    //  Distrib. Option / MCMC Chains
    //  Note : mChainChecks and mChainRadios are populated by createChainsControls()
    // ------------------------------------
    mChainsTitle = new Label(tr("MCMC Chains"), this);
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
    //  Distrib. Option / Density Options
    // ------------------------------------
    mDensityOptsTitle = new Label(tr("Density Options"), this);
    mDensityOptsTitle->setFixedHeight(25);
    mDensityOptsTitle->setIsTitle(true);

    mDensityOptsGroup = new QWidget();

    mCredibilityCheck = new CheckBox(tr("Show Confidence Bar"));
    mCredibilityCheck->setFixedHeight(16);
    mCredibilityCheck->setChecked(true);

    mThreshLab = new QLabel(tr("Confidence Level (%)"), mDensityOptsGroup);
    mThreshLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mThresholdEdit = new LineEdit(mDensityOptsGroup);
    DoubleValidator* percentValidator = new DoubleValidator();
    percentValidator->setBottom(0.0);
    percentValidator->setTop(100.0);
    //percentValidator->setDecimals(1);
    mThresholdEdit->setValidator(percentValidator);
    mThresholdEdit->setFixedHeight(16);

    // Used with Activity
    mRangeThreshLab = new QLabel(tr("Time Range Level (%)"), mDensityOptsGroup);
    mRangeThreshLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mRangeThresholdEdit = new LineEdit(mDensityOptsGroup);
   // DoubleValidator* percentValidator = new DoubleValidator();
   // percentValidator->setBottom(0.0);
   // percentValidator->setTop(100.0);
    //percentValidator->setDecimals(1);
    mRangeThresholdEdit->setValidator(percentValidator);
    mRangeThresholdEdit->setFixedHeight(16);


    mFFTLenLab = new QLabel(tr("Grid Length"), mDensityOptsGroup);
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
    mFFTLenCombo->setFixedHeight(16);
    mFFTLenCombo->setCurrentIndex(5);

    mBandwidthLab = new QLabel(tr("FFTW Bandwidth"), mDensityOptsGroup);
    mBandwidthLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mBandwidthSpin = new QDoubleSpinBox(mDensityOptsGroup);
    mBandwidthSpin->setDecimals(2);
    mBandwidthSpin->setFixedHeight(20);

    mHActivityLab = new QLabel(tr("Activity Bandwidth"), mDensityOptsGroup);
    mHActivityEdit = new LineEdit(mDensityOptsGroup);
    mHActivityEdit->setFixedHeight(16);
    
    connect(mCredibilityCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mFFTLenCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ResultsView::applyFFTLength);
    connect(mBandwidthSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &ResultsView::applyBandwidth);
    connect(mThresholdEdit, &LineEdit::editingFinished, this, &ResultsView::applyThreshold);

    connect(mRangeThresholdEdit, &LineEdit::editingFinished, this, &ResultsView::applyHActivity);
    connect(mHActivityEdit, &LineEdit::editingFinished, this, &ResultsView::applyHActivity);

    QHBoxLayout* densityLayout1 = new QHBoxLayout();
    densityLayout1->addWidget(mRangeThreshLab);
    densityLayout1->addWidget(mRangeThresholdEdit);

    QHBoxLayout* densityLayout2 = new QHBoxLayout();
   // densityLayout1->setContentsMargins(0, 0, 0, 0);
    densityLayout2->addWidget(mThreshLab);
    densityLayout2->addWidget(mThresholdEdit);

    QHBoxLayout* densityLayout3 = new QHBoxLayout();
  //  densityLayout4->setContentsMargins(0, 0, 0, 0);
    densityLayout3->addWidget(mHActivityLab);
    densityLayout3->addWidget(mHActivityEdit);

    QHBoxLayout* densityLayout4 = new QHBoxLayout();
   // densityLayout2->setContentsMargins(0, 0, 0, 0);
    densityLayout4->addWidget(mFFTLenLab);
    densityLayout4->addWidget(mFFTLenCombo);

    QHBoxLayout* densityLayout5 = new QHBoxLayout();
  //  densityLayout3->setContentsMargins(0, 0, 0, 0);
    densityLayout5->addWidget(mBandwidthLab);
    densityLayout5->addWidget(mBandwidthSpin);

   // spanLayout->setContentsMargins(10, 10, 10, 10);
   // spanLayout->setSpacing(5);
    
    QVBoxLayout* densityLayout = new QVBoxLayout();
    densityLayout->setContentsMargins(10, 0, 0, 0);
    densityLayout->setSpacing(5);
    densityLayout->addWidget(mCredibilityCheck);
    densityLayout->addLayout(densityLayout1);
    densityLayout->addLayout(densityLayout2);
    densityLayout->addLayout(densityLayout3);
    densityLayout->addLayout(densityLayout4);
    densityLayout->addLayout(densityLayout5);

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
    //  Tab Page
    // ------------------------------------
    const qreal layoutWidth = mOptionsW;
    const qreal internSpacing = 2;

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

    /* Using QLayout does not work with mPageEdit and Button widgets.
     * That's why it's faster to use directly setGeometry()
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
    mExportResults->setIcon(QIcon(":export_csv_w.png"));
    mExportResults->setToolTip(tr("Export all result in several files"));

    connect(mExportImgBut, static_cast<void (Button::*)(bool)>(&Button::clicked), this, &ResultsView::exportFullImage);
    connect(mExportResults, static_cast<void (Button::*)(bool)>(&Button::clicked), this, &ResultsView::exportResults);

    const qreal layoutWidthBy2 = (layoutWidth - internSpacing)/2;
    mExportImgBut->setGeometry(0, 0, layoutWidthBy2, 33);
    mExportResults->setGeometry(layoutWidthBy2 + internSpacing, 0, layoutWidthBy2, 33);

    mSaveAllWidget->setFixedSize(layoutWidth, 33);

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

    const qreal layoutWidthBy4 = (layoutWidth - 4*internSpacing)/4.;

    mImageSaveBut->setGeometry(0, 0, layoutWidthBy4, 33);

    mImageClipBut->setGeometry(mImageSaveBut->x()+mImageSaveBut->width() + internSpacing, 0, layoutWidthBy4, 33);

    mResultsClipBut->setGeometry(mImageClipBut->x()+mImageClipBut->width() + internSpacing, 0, layoutWidthBy4, 33);

    mDataSaveBut->setGeometry(mResultsClipBut->x()+mResultsClipBut->width() + internSpacing, 0, layoutWidthBy4 , 33);


    mSaveSelectWidget->setFixedSize(layoutWidth, 33);

    connect(mImageSaveBut, static_cast<void (Button::*)(bool)>(&Button::clicked), this, &ResultsView::saveAsImage);
    connect(mImageClipBut, static_cast<void (Button::*)(bool)>(&Button::clicked), this, &ResultsView::imageToClipboard);
    connect(mResultsClipBut, static_cast<void (Button::*)(bool)>(&Button::clicked), this, &ResultsView::resultsToClipboard);
    connect(mDataSaveBut, static_cast<void (Button::*)(bool)>(&Button::clicked), this, &ResultsView::saveGraphData);

    // ------------------------------------
    //  Page / Saving
    // ------------------------------------

    mPageSaveTab = new Tabs(mOptionsWidget);
    mPageSaveTab->addTab(tr("Page"));
    mPageSaveTab->addTab(tr("Saving"));
    mPageSaveTab->setFixedHeight(mPageSaveTab->tabHeight());

    mPageSaveTab->setTab(0, false);

    connect(mPageSaveTab, static_cast<void (Tabs::*)(const int&)>(&Tabs::tabClicked), this, &ResultsView::updateOptionsWidget);

    // ---------------------------------------------------------------------------
    //  Right Layout (Options)
    // ---------------------------------------------------------------------------
    mOptionsLayout = new QVBoxLayout();
    mOptionsLayout->setContentsMargins(mMargin, mMargin, 0, 0);
   // mOptionsLayout->setSpacing(2);
    mOptionsLayout->addWidget(mGraphListTab);
    mOptionsLayout->addWidget(mEventsGroup);
    mOptionsLayout->addWidget(mPhasesGroup);
    mOptionsLayout->addWidget(mCurvesGroup);

    //mOptionsLayout->setSpacing(2);
    mOptionsLayout->addWidget(mDisplayDistribTab);
    mOptionsLayout->addWidget(mDisplayWidget);

    //mOptionsLayout->addSpacing(2);
    mOptionsLayout->addWidget(mPageSaveTab);
    mOptionsLayout->addWidget(mPageWidget);


    mSaveAllWidget->setVisible(true);
    mSaveSelectWidget->setVisible(true);

    mDistribWidget->setVisible(true);

    mOptionsLayout->addStretch();
    mOptionsWidget->setLayout(mOptionsLayout);




    // ---------------------------------------------------------------------------
    //  Inititialize tabs indexes
    // ---------------------------------------------------------------------------
    mGraphTypeTabs->setTab(0, false);
    mGraphListTab->setTab(0, false);
    mDisplayDistribTab->setTab(0, false);

    mPageSaveTab->setTab(0, false);

    mEventsScrollArea->setVisible(true);
    mPhasesScrollArea->setVisible(false);
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
    initModel(project->mModel);
    connect(project, &Project::mcmcStarted, this, &ResultsView::clearResults);
}

void ResultsView::clearResults()
{
   // deleteChainsControls();
    deleteAllGraphsInList(mByEventsGraphs);
    deleteAllGraphsInList(mByPhasesGraphs);
    deleteAllGraphsInList(mByCurveGraphs);
}

void ResultsView::updateModel(Model* model)
{
    (void) model;
    createGraphs();
    updateLayout();

}

void ResultsView::initModel(Model* model)
{
    mModel = model;
    connect(mModel, &Model::newCalculus, this, &ResultsView::generateCurves);

    Scale timeScale;
    timeScale.findOptimalMark(mModel->mSettings.getTminFormated(), mModel->mSettings.getTmaxFormated(), 7);
    mMajorScale = timeScale.mark;
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
    mAllChainsCheck->setChecked(true);

    mCurrentTypeGraph = GraphViewResults::ePostDistrib;
    mCurrentVariableList.clear();
    if (isCurve()) {
        const ModelCurve* modelCurve = static_cast<const ModelCurve*> (model);
        mMainVariable = GraphViewResults::eG;
        mCurveGRadio->setChecked(true);
        mGraphListTab->setTab(2, false);

        Scale XScale;
        XScale.findOptimal(modelCurve->mPosteriorMeanG.gx.vecG.front(), modelCurve->mPosteriorMeanG.gx.vecG.back(), 7);
        mResultCurrentMinX = XScale.min;
        mResultCurrentMaxX = XScale.max;
        applyXRange();

        mXOptionTitle->setText(mModel->getCurvesLongName().at(0) + " " + tr("Scale"));
        mXOptionLab->setText(mModel->getCurvesName().at(0));
        mXOptionBut->setText(tr("Optimal") + " " + mModel->getCurvesName().at(0) + " " + tr("Display"));

        if (mModel->displayY() && !modelCurve->mPosteriorMeanG.gy.vecG.empty() ) {
            XScale.findOptimal(modelCurve->mPosteriorMeanG.gy.vecG.front(), modelCurve->mPosteriorMeanG.gy.vecG.back(), 7);
            mResultCurrentMinY = XScale.min;
            mResultCurrentMaxY = XScale.max;
            applyYRange();
            mYOptionTitle->setText(mModel->getCurvesLongName().at(1) + " " + tr("Scale"));
            mYOptionLab->setText(mModel->getCurvesName().at(1));
            mYOptionBut->setText(tr("Optimal") + " " + mModel->getCurvesName().at(1) + " " + tr("Display"));

            if (mModel->displayZ() && !modelCurve->mPosteriorMeanG.gz.vecG.empty() ) {
                XScale.findOptimal(modelCurve->mPosteriorMeanG.gz.vecG.front(), modelCurve->mPosteriorMeanG.gz.vecG.back(), 7);
                mResultCurrentMinZ = XScale.min;
                mResultCurrentMaxZ = XScale.max;
                applyZRange();
                mZOptionTitle->setText(mModel->getCurvesLongName().at(2) + " " + tr("Scale"));
                mZOptionLab->setText(mModel->getCurvesName().at(2));
                mZOptionBut->setText(tr("Optimal") + " " + mModel->getCurvesName().at(2) + " " + tr("Display"));
            }
        }

    } else if (mHasPhases) {
        mMainVariable = GraphViewResults::eBeginEnd;
        mGraphListTab->setTab(1, false);

    } else {
        mMainVariable = GraphViewResults::eThetaEvent;
        mGraphListTab->setTab(0, false);
    }
    updateMainVariable();
    mCurrentVariableList.append(mMainVariable);
    mRangeThresholdEdit->setText(stringForLocal(95.));
    mThresholdEdit->setText(stringForLocal(mModel->getThreshold()));
    mHActivityEdit->setText(stringForLocal(mModel->mHActivity));

    mFFTLenCombo->setCurrentText(stringForLocal(mModel->getFFTLength()));
    mBandwidthSpin->setValue(mModel->getBandwidth());

    applyStudyPeriod();
    updateOptionsWidget();
    createGraphs();
    updateLayout();

    showStats(mStatCheck->isChecked());
}

#pragma mark Layout

void ResultsView::mouseMoveEvent(QMouseEvent* e)
{
    updateMarkerGeometry(e->pos().x());
}

void ResultsView::resizeEvent(QResizeEvent* e)
{
    (void) e;
    updateLayout();
}

void ResultsView::updateMarkerGeometry(const int x)
{
    const int markerXPos = std::clamp(0, x, mRuler->x() + mRuler->width());
    mMarker->setGeometry(markerXPos, mGraphTypeTabs->height() + mMargin, mMarker->thickness(), height() - mGraphTypeTabs->height() - mMargin);
    update(markerXPos -5, mGraphTypeTabs->height() + mMargin, 10, mGraphTypeTabs->height() + mMargin);
}

void ResultsView::updateLayout()
{    
    // --------------------------------------------------------
    //  Right layout
    // --------------------------------------------------------
    mOptionsScroll->setGeometry(width() - mOptionsW - mMargin/2, 0, mOptionsW, height());

    // ----------------------------------------------------------
    //  Left layout
    // ----------------------------------------------------------
    const int leftWidth = width() - mOptionsScroll->width() - 2*mMargin;
    const int tabsH = mGraphTypeTabs->tabHeight();

    int graphWidth = leftWidth;  // to adjust the ruler length
    if (mStatCheck->isChecked() || mPhasesStatCheck->isChecked() || mCurveStatCheck->isChecked()) {
        graphWidth = (2./3.) * leftWidth;
    }

    mGraphTypeTabs->setGeometry(mMargin, mMargin, leftWidth, tabsH);
    mRuler->setGeometry(mMargin, mMargin + tabsH, graphWidth, Ruler::sHeight);

    const int stackH = height() - mMargin - tabsH - Ruler::sHeight;
    const QRect graphScrollGeometry(mMargin, mMargin + tabsH + Ruler::sHeight, leftWidth, stackH);

    mEventsScrollArea->setGeometry(graphScrollGeometry);
    mPhasesScrollArea->setGeometry(graphScrollGeometry);
    mCurveScrollArea->setGeometry(graphScrollGeometry);

    updateGraphsLayout();
    updateMarkerGeometry(mMarker->pos().x());
}


void ResultsView::updateGraphsLayout()
{
    // Display the scroll area corresponding to the selected tab
    mEventsScrollArea->setVisible(mGraphListTab->currentIndex() == 0);
    mPhasesScrollArea->setVisible(mGraphListTab->currentIndex() == 1);
    mCurveScrollArea->setVisible(mGraphListTab->currentIndex() == 2);

    if (mGraphListTab->currentIndex() == 0) {
        updateGraphsLayout(mEventsScrollArea, mByEventsGraphs);

    } else if (mGraphListTab->currentIndex() == 1) {
        updateGraphsLayout(mPhasesScrollArea, mByPhasesGraphs);

    } else if (mGraphListTab->currentIndex() == 2) {
        updateGraphsLayout(mCurveScrollArea, mByCurveGraphs);
    }

}


void safe_update_graph(std::mutex &mutex_g, GraphViewResults* g)
{
    const std::lock_guard<std::mutex> lock(mutex_g);
    //std::thread ([](GraphViewResults* gv) {
                                            g->setVisible(true);
                                            g->update();
   //                                        }, g );

    // g_mutex is automatically released when lock
    // goes out of scope
}


void ResultsView::updateGraphsLayout(QScrollArea* scrollArea, QList<GraphViewResults*> graphs)
{
    QWidget* widget = scrollArea->widget();

    // Used to magnify the graph for curve
    int coefDisplay = 1;
    if (mGraphListTab->currentIndex() == 2 )
        coefDisplay = 3;

    // Rq: Le widget doit être plus petit que le scrollArea pour ne pas bouger horizontalement!,
    //par contre le graph à l'interieur du widget peut avoir la même taille
    if (widget) {                
        widget->resize(scrollArea->width()-2, graphs.size() * mGraphHeight * coefDisplay);

        int i = 0;
        for (auto&& g : graphs) {
            g->setGeometry(0, (i++) * mGraphHeight*coefDisplay, scrollArea->width(), mGraphHeight * coefDisplay);
            g->setVisible(true);
            g->update();
        }

/*
        std::vector<std::thread> queue;
        std::mutex g_mutex;  // protects g, utilise la même memoire graphique
        for (auto g : graphs) {
            queue.push_back( std::thread ([&g_mutex] (GraphViewResults* g) {
                                                       const std::lock_guard<std::mutex> lock(g_mutex);
                                                        g->setVisible(true);
                                                        g->update();}
            ,  g) );
        };

        for (auto&& th : queue) {
            th.join();
        }
*/
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
    mCurrentTypeGraph = (GraphViewResults::graph_t) mGraphTypeTabs->currentIndex();

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
    mCurveScrollArea->setVisible(currentIndex == 2);
    
    // Update the current variable to the most appropriate for this list :
    if (currentIndex == 0) {
        mMainVariable = GraphViewResults::eThetaEvent;
        mEventThetaRadio->setChecked(true);

    } else if (currentIndex == 1) {
        mMainVariable = GraphViewResults::eBeginEnd;
        mBeginEndRadio->setChecked(true);

    } else if (currentIndex == 2) {
        mMainVariable = GraphViewResults::eG;
        mCurveGRadio->setChecked(true);
    }

    // Set the current graph type to Posterior distrib :
    mGraphTypeTabs->setTab(0, false);
    mCurrentTypeGraph = (GraphViewResults::graph_t) mGraphTypeTabs->currentIndex();
    
    // Changing the graphs list implies to go back to page 1 :
    mCurrentPage = 0;
    
    applyCurrentVariable();

}

void ResultsView::updateMainVariable()
{
    mCurrentVariableList.clear();

    if (mGraphListTab->currentName() == tr("Events")) {
        if (mEventThetaRadio->isChecked()) {
            mMainVariable = GraphViewResults::eThetaEvent;
            if (mEventsDatesUnfoldCheck->isChecked()) {
                mCurrentVariableList.append(GraphViewResults::eDataTi);

                if (mDataCalibCheck->isChecked())
                    mCurrentVariableList.append(GraphViewResults::eDataCalibrate);

                if (mWiggleCheck->isChecked())
                    mCurrentVariableList.append(GraphViewResults::eDataWiggle);

            }

        } else if (mDataSigmaRadio->isChecked()) {
            mMainVariable = GraphViewResults::eSigma;

        } else if (mEventVGRadio->isChecked()) {
            mMainVariable = GraphViewResults::eVg;
        }

    } else if (mGraphListTab->currentName() == tr("Phases")) {

        if (mBeginEndRadio->isChecked()) {
            mMainVariable = GraphViewResults::eBeginEnd;

            if (mPhasesEventsUnfoldCheck->isChecked()) {
                mCurrentVariableList.append(GraphViewResults::eThetaEvent);

                if (mPhasesDatesUnfoldCheck->isChecked()) {
                    mCurrentVariableList.append(GraphViewResults::eDataTi);

                  /*  if (mDataCalibCheck->isChecked())
                        mCurrentVariableList.append(GraphViewResults::eDataCalibrate);

                    if (mWiggleCheck->isChecked())
                        mCurrentVariableList.append(GraphViewResults::eDataWiggle);*/
                }
            }


        }  else if (mTempoRadio->isChecked()) {
            mMainVariable = GraphViewResults::eTempo;

            if (mPhasesEventsUnfoldCheck->isChecked()) {
                mCurrentVariableList.append(GraphViewResults::eThetaEvent);

                if (mPhasesDatesUnfoldCheck->isChecked()) {
                    mCurrentVariableList.append(GraphViewResults::eDataTi);

                  /*  if (mDataCalibCheck->isChecked())
                        mCurrentVariableList.append(GraphViewResults::eDataCalibrate);

                    if (mWiggleCheck->isChecked())
                        mCurrentVariableList.append(GraphViewResults::eDataWiggle);*/
                }
              }


        } else  if (mActivityRadio->isChecked()) {
            mMainVariable = GraphViewResults::eActivity;

            if (mPhasesEventsUnfoldCheck->isChecked()) {
                mCurrentVariableList.append(GraphViewResults::eThetaEvent);

                if (mPhasesDatesUnfoldCheck->isChecked()) {
                    mCurrentVariableList.append(GraphViewResults::eDataTi);

                  /*  if (mDataCalibCheck->isChecked())
                        mCurrentVariableList.append(GraphViewResults::eDataCalibrate);

                    if (mWiggleCheck->isChecked())
                        mCurrentVariableList.append(GraphViewResults::eDataWiggle);*/
                }
            }

        } else  if (mDurationRadio->isChecked()) {
            mMainVariable = GraphViewResults::eDuration;

        }

    } else if (mGraphListTab->currentName() == tr("Curves")) {

        if (mLambdaRadio->isChecked()) {
            mMainVariable = GraphViewResults::eLambda;

        } else if (mS02VgRadio->isChecked()) {
            mMainVariable = GraphViewResults::eS02Vg;

        } else if (mCurveGRadio->isChecked()) {
            mMainVariable = GraphViewResults::eG;
            if (mCurveErrorCheck->isChecked())
                mCurrentVariableList.append(GraphViewResults::eGError);

            if (mCurveMapCheck->isChecked())
                mCurrentVariableList.append(GraphViewResults::eMap);

            if (mCurveEventsPointsCheck->isChecked())
                mCurrentVariableList.append(GraphViewResults::eGEventsPts);

            if (mCurveDataPointsCheck->isChecked())
                mCurrentVariableList.append(GraphViewResults::eGDatesPts);

            mCurrentTypeGraph = GraphViewResults::ePostDistrib;
            mGraphTypeTabs->setTab(0, false);

        } else if (mCurveGPRadio->isChecked()) {
            mMainVariable = GraphViewResults::eGP;
            if (mCurveErrorCheck->isChecked())
                mCurrentVariableList.append(GraphViewResults::eGError);

            mCurrentTypeGraph = GraphViewResults::ePostDistrib;
            mGraphTypeTabs->setTab(0, false);

        } else if (mCurveGSRadio->isChecked()) {
            mMainVariable = GraphViewResults::eGS;
            mCurrentTypeGraph = GraphViewResults::ePostDistrib;
            mGraphTypeTabs->setTab(0, false);
        }

    }

    mCurrentVariableList.append(mMainVariable);
}

void ResultsView::applyCurrentVariable()
{
    updateMainVariable();
    createGraphs();

    updateOptionsWidget();

    updateLayout();
}

#pragma mark Chains controls

void ResultsView::toggleDisplayDistrib()
{
    const bool isPostDistrib = isPostDistribGraph();
    // -------------------------------------------------------------------------------------
    //  MCMC Display options are not visible for mCurrentVariable = Tempo
    // -------------------------------------------------------------------------------------
    if ( mMainVariable == GraphViewResults::eTempo ||
         mMainVariable == GraphViewResults::eGP ||
         mMainVariable == GraphViewResults::eGS) {

        mDisplayDistribTab->setTabVisible(1, false);
        mDisplayDistribTab->setTab(0, false);

    } else {
        mDisplayDistribTab->setTabVisible(1, true);
    }

    const bool displayX = mModel->displayX();
    const bool displayY = mModel->displayY();
    const bool displayZ = mModel->displayZ();
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

        widHeigth = 11*h + 12*internSpan;

        mDisplayStudyBut->setText(xScaleRepresentsTime() ? tr("Study Period Display") : tr("Fit Display"));
        mDisplayStudyBut->setVisible(true);
        widHeigth += mDisplayStudyBut->height() + internSpan;


        if (widFrom != mDisplayWidget)
            mOptionsLayout->replaceWidget(widFrom, mDisplayWidget);

        if (isCurve() && ( mMainVariable == GraphViewResults::eG ||
                           mMainVariable == GraphViewResults::eGP ||
                           mMainVariable == GraphViewResults::eGS)) {
            mXOptionTitle->setVisible(displayX);
            mXOptionGroup->setVisible(displayX);
            widHeigth += displayX ? mXOptionGroup->height() + mXOptionTitle->height() : 0.;

            mYOptionTitle->setVisible(displayY);
            mYOptionGroup->setVisible(displayY);
            widHeigth += displayY ?mYOptionGroup->height() + mYOptionTitle->height() : 0.;

            mZOptionTitle->setVisible(displayZ);
            mZOptionGroup->setVisible(displayZ);
            mDisplayWidget->setFixedHeight(widHeigth + (displayZ ? mZOptionGroup->height() + mZOptionTitle->height(): 0.)) ;

        } else {
            mXOptionTitle->setVisible(false);
            mXOptionGroup->setVisible(false);

            mYOptionTitle->setVisible(false);
            mYOptionGroup->setVisible(false);

            mYOptionTitle->setVisible(false);
            mYOptionGroup->setVisible(false);
            mDisplayWidget-> setFixedHeight(widHeigth);
        }

    } else { // Tab Distrib. Option
        mDisplayWidget->setVisible(false);
        mDistribWidget->setVisible(true);
        widHeigth = 0;
        const int internSpan = 5;
        // ------------------------------------
        //  MCMC Chains
        //  Switch between checkBoxes or Radio-buttons for chains
        // ------------------------------------
        mAllChainsCheck->setVisible(isPostDistrib);

        if (mAllChainsCheck->isVisible())
            widHeigth += internSpan + mAllChainsCheck->height() + internSpan;

        for (auto&& checkChain : mChainChecks) {
            checkChain->setVisible(isPostDistrib);
            if (checkChain->isVisible())
                widHeigth += checkChain->height() + internSpan;
        }

        for (auto&& chainRadio : mChainRadios) {
            chainRadio->setVisible(!isPostDistrib);
            if (chainRadio->isVisible())
                widHeigth += chainRadio->height() + internSpan;
        }

        mChainsGroup->setFixedHeight(widHeigth);
        widHeigth += mChainsTitle->height();
        // ------------------------------------
        //  Density Options
        // ------------------------------------
        bool showDensityOptions = isPostDistrib && (mMainVariable != GraphViewResults::eTempo
                && mMainVariable != GraphViewResults::eG
                && mMainVariable != GraphViewResults::eGP
                && mMainVariable != GraphViewResults::eGS);


        mDensityOptsTitle->setVisible(showDensityOptions);
        mDensityOptsGroup->setVisible(showDensityOptions);
        if (showDensityOptions) {

            mDensityOptsGroup->setFixedHeight( mCredibilityCheck->height() + mThresholdEdit->height() + mFFTLenCombo->height()
                                              + mBandwidthSpin->height() + mHActivityEdit->height() + mRangeThresholdEdit->height()
                                               + 12* internSpan);

            widHeigth += mDensityOptsTitle->height() + mDensityOptsGroup->height();



        }
        widHeigth += 4*internSpan;
        mDistribWidget-> setFixedHeight(widHeigth);
        if (widFrom != mDistribWidget)
            mOptionsLayout->replaceWidget(widFrom, mDistribWidget);
    }

}

void ResultsView::createChainsControls()
{
    if (mModel->mChains.size() != mChainChecks.size()) {
        deleteChainsControls();

        for (int i=0; i<mModel->mChains.size(); ++i) {
            CheckBox* check = new CheckBox(tr("Chain %1").arg(QString::number(i+1)), mChainsGroup);
            check->setFixedHeight(16);
            check->setVisible(true);
            mChainChecks.append(check);

            connect(check, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);

            RadioButton* radio = new RadioButton(tr("Chain %1").arg(QString::number(i+1)), mChainsGroup);
            radio->setFixedHeight(16);
            radio->setChecked(i == 0);
            radio->setVisible(true);
            mChainRadios.append(radio);

            connect(radio, &RadioButton::clicked, this, &ResultsView::updateCurvesToShow);

            mChainsGroup->layout()->addWidget(check);
            mChainsGroup->layout()->addWidget(radio);
        }
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

    if (mGraphListTab->currentName() == tr("Events")) {
        createByEventsGraphs();

    } else if (mGraphListTab->currentName() == tr("Phases")) {
        createByPhasesGraphs();

    } else if (mGraphListTab->currentName() == tr("Curves")) {
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
    
    if (mGraphListTab->currentName() == tr("Events")) {
        bool showAllEvents = ! mModel->hasSelectedEvents();
        for (const auto& event : mModel->mEvents) {
            if (event->mIsSelected || showAllEvents) {
                ++totalGraphs;
                
                if (mEventsDatesUnfoldCheck->isChecked())
                    totalGraphs += event->mDates.size();

            }
        }
    } else if (mGraphListTab->currentName() == tr("Phases")) {
        bool showAllPhases = ! mModel->hasSelectedPhases();

        for (const auto& phase : mModel->mPhases) {
            if (phase->mIsSelected || showAllPhases) {
                ++totalGraphs;
                
                if (mPhasesEventsUnfoldCheck->isChecked())  {
                    for (const auto& event : phase->mEvents) {
                        ++totalGraphs;

                        if (mPhasesDatesUnfoldCheck->isChecked())
                            totalGraphs += event->mDates.size();

                    }
                }
            }
        }
    } else if (mGraphListTab->currentName() == tr("Curves")) {
        if (mLambdaRadio->isChecked()) {
            ++totalGraphs;

        } else {
            if (!mModel->mEvents.isEmpty() && isCurve()) {
                ++totalGraphs;
                if (mModel->displayY()) ++totalGraphs;
                if (mModel->displayZ()) ++totalGraphs;
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

    for (const auto& event : mModel->mEvents) {
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
                connect(graph, &GraphViewResults::selected, this, &ResultsView::updateOptionsWidget);


            }
            ++graphIndex;
                
            if (mEventsDatesUnfoldCheck->isChecked()) {
                for (auto&& date : event->mDates) {
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
                        connect(graph, &GraphViewResults::selected, this, &ResultsView::updateOptionsWidget);
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
    const bool showAllPhases = ! mModel->hasSelectedPhases();

    // ----------------------------------------------------------------------
    //  Iterate through all, and create corresponding graphs
    // ----------------------------------------------------------------------
    QWidget* phasesWidget = mPhasesScrollArea->widget();
    int graphIndex = 0;

    for (const auto& phase : mModel->mPhases) {
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
                connect(graph, &GraphViewResults::selected, this, &ResultsView::updateOptionsWidget);
            }
            ++graphIndex;
            
            if (mPhasesEventsUnfoldCheck->isChecked()) {
                for (const auto& event : phase->mEvents) {
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
                        connect(graph, &GraphViewResults::selected, this, &ResultsView::updateOptionsWidget);
                    }
                    ++graphIndex;
                        
                    if (mPhasesDatesUnfoldCheck->isChecked()) {
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
                                connect(graph, &GraphViewResults::selected, this, &ResultsView::updateOptionsWidget);
                            }
                            ++graphIndex;
                        }
                    }
                }
            }
        }
    }
}

void ResultsView::createByCurveGraph()
{
   if (!isCurve())
        return;

    ModelCurve* model = modelCurve();
    // ----------------------------------------------------------------------
    // Show all events unless at least one is selected
    // ----------------------------------------------------------------------
    bool showAllEvents = ! mModel->hasSelectedEvents();

    // ----------------------------------------------------------------------
    //  Disconnect and delete existing graphs
    // ----------------------------------------------------------------------
    deleteAllGraphsInList(mByCurveGraphs);
    
    QWidget* widget = mCurveScrollArea->widget();

    if (mLambdaRadio->isChecked())  {
        GraphViewLambda* graphAlpha = new GraphViewLambda(widget);
        graphAlpha->setSettings(mModel->mSettings);
        graphAlpha->setMCMCSettings(mModel->mMCMCSettings, mModel->mChains);
        graphAlpha->setGraphsFont(mFontBut->font());
        graphAlpha->setGraphsThickness(mThicknessCombo->currentIndex());
        graphAlpha->changeXScaleDivision(mMajorScale, mMinorCountScale);
        graphAlpha->setMarginLeft(mMarginLeft);
        graphAlpha->setMarginRight(mMarginRight);
        graphAlpha->setTitle(tr("Smoothing"));
        graphAlpha->setModel(model);

        mByCurveGraphs.append(graphAlpha);

        connect(graphAlpha, &GraphViewResults::selected, this, &ResultsView::updateOptionsWidget);

    } else  if (mS02VgRadio->isChecked())  {
        GraphViewS02* graphS02 = new GraphViewS02(widget);
        graphS02->setSettings(mModel->mSettings);
        graphS02->setMCMCSettings(mModel->mMCMCSettings, mModel->mChains);
        graphS02->setGraphsFont(mFontBut->font());
        graphS02->setGraphsThickness(mThicknessCombo->currentIndex());
        graphS02->changeXScaleDivision(mMajorScale, mMinorCountScale);
        graphS02->setMarginLeft(mMarginLeft);
        graphS02->setMarginRight(mMarginRight);
        graphS02->setTitle(tr("Shrinkage param."));
        graphS02->setModel(model);

        mByCurveGraphs.append(graphS02);

        connect(graphS02, &GraphViewResults::selected, this, &ResultsView::updateOptionsWidget);

    } else  {
        const bool displayY = model->displayY();
        const bool displayZ = model->displayZ();

        // insert refpoints for X
      //  const double thresh = 68.4; //80;
        QVector<CurveRefPts> eventsPts;
        QVector<CurveRefPts> dataPts;
        // Stock the numbre of ref points per Event and Data
        std::vector<int> dataPerEvent;
        std::vector<int> hpdPerEvent;
        if (mMainVariable == GraphViewResults::eG) {

            //Same calcul within MultiCalibrationView::scatterPlot(const double thres)
            // Creation des points de ref
            for (const auto& event : modelCurve()->mEvents) {
                if (event->mIsSelected || showAllEvents) {
                    CurveRefPts evPts;
                    CurveRefPts dPts;
                    double verr;
                    // Set Y
                    if (!displayY) {
                        switch (model->mCurveSettings.mVariableType) {
                        case CurveSettings::eVariableTypeInclination :
                            evPts.Ymax = event->mXIncDepth + event->mS_XA95Depth;
                            evPts.Ymin = event->mXIncDepth - event->mS_XA95Depth;
                            break;
                        case CurveSettings::eVariableTypeDeclination :
                            verr = event->mS_XA95Depth / cos(event->mXIncDepth * M_PI /180.);
                            evPts.Ymin = event->mYDec - verr;
                            evPts.Ymax = event->mYDec + verr;
                            break;
                        case CurveSettings::eVariableTypeField :
                            evPts.Ymin = event->mZField - 1.96*event->mS_ZField;
                            evPts.Ymax = event->mZField + 1.96*event->mS_ZField;
                            break;
                        case CurveSettings::eVariableTypeDepth :
                            evPts.Ymin = event->mXIncDepth - 1.96*event->mS_XA95Depth;
                            evPts.Ymax = event->mXIncDepth + 1.96*event->mS_XA95Depth;
                            break;
                        case CurveSettings::eVariableTypeOther :
                            evPts.Ymin = event->mXIncDepth - 1.96*event->mS_XA95Depth;
                            evPts.Ymax = event->mXIncDepth + 1.96*event->mS_XA95Depth;
                            break;
                        }

                    } else {
                        //must be inclination or X
                        evPts.Ymin = event->mXIncDepth - 1.96*event->mS_XA95Depth;
                        evPts.Ymax = event->mXIncDepth + 1.96*event->mS_XA95Depth;
                    }
                    evPts.color = event->mColor;

                    // Set X = time

                    if (event->mType == Event::eDefault) {
                        int nb_dataPts = 0;
                        for (const auto& date: event->mDates) {
                            // --- Calibration Date

                            QMap<double, double> calibMap = date.getRawCalibMap();
                            // hpd is calculate only on the study Period

                            //const QMap<double, double> &subData = getMapDataInRange(calibMap, mModel->mSettings.getTminFormated(), mModel->mSettings.getTmaxFormated());

                            //if (!subData.isEmpty()) {

                                // hpd results

                            const QMap<double, double> hpd (create_HPD2(calibMap, date.mTi.mThresholdUsed));

                            const QList<QPair<double, QPair<double, double> > > &intervals = intervalsForHpd(hpd, 100);

                            // ---
                          // -- Post Distrib of Ti
                           // const QList<QPair<double, QPair<double, double> > > &intervals = date.mTi.mRawHPDintervals;

                            if (intervals.size() > 1) {
                               // dataPerEvent.push_back(intervals.size() + 1);
                                for (const auto& h : intervals) {
                                    dPts.Xmin = h.second.first;
                                    dPts.Xmax = h.second.second;
                                    dPts.Ymin = evPts.Ymin;
                                    dPts.Ymax = evPts.Ymax;
                                    dPts.color = event->mColor;
                                    dPts.type = CurveRefPts::eLine;
                                    // memo Data Points
                                    nb_dataPts++;
                                    dataPts.append(dPts);
                                }
                                dPts.Xmin = intervals.first().second.first;
                                dPts.Xmax = intervals.last().second.second;
                                dPts.Ymin = evPts.Ymin;
                                dPts.Ymax = evPts.Ymax;
                                dPts.color = event->mColor;
                                dPts.type = CurveRefPts::eDotLineCross;
                                // memo Data Points
                                nb_dataPts++;
                                dataPts.append(dPts);


                            } else if (intervals.size() == 1) {
                                //dataPerEvent.push_back(1);
                                dPts.Xmin = intervals.first().second.first;
                                dPts.Xmax = intervals.last().second.second;
                                dPts.Ymin = evPts.Ymin;
                                dPts.Ymax = evPts.Ymax;
                                dPts.color = event->mColor;
                                dPts.type = CurveRefPts::eCross;
                                // memo Data Points
                                nb_dataPts++;
                                dataPts.append(dPts);
                            }

                        }
                        dataPerEvent.push_back(nb_dataPts);

                        const QList<QPair<double, QPair<double, double> > > &intervals = event->mTheta.mRawHPDintervals;

                        for (const auto& h : intervals) {
                            evPts.Xmin =  h.second.first;
                            evPts.Xmax =  h.second.second;
                            evPts.Ymin = evPts.Ymin;
                            evPts.Ymax = evPts.Ymax;
                            evPts.color = event->mColor;
                            evPts.type = CurveRefPts::eLine;
                            // memo Data Points
                            eventsPts.append(evPts);
                        }

                        if (intervals.size() > 1) {
                            evPts.Xmin = intervals.first().second.first;
                            evPts.Xmax = intervals.last().second.second;
                            evPts.Ymin = evPts.Ymin;
                            evPts.Ymax = evPts.Ymax;
                            evPts.color = event->mColor;
                            evPts.type = CurveRefPts::eDotLine;
                            // memo Data Points
                            eventsPts.append(evPts);

                            hpdPerEvent.push_back(intervals.size() + 1);

                        } else if (intervals.size() == 1) {
                            hpdPerEvent.push_back(1);
                        }


                    } else {

                        evPts.Xmin = static_cast<Bound*>(event)->mFixed;
                        evPts.Xmax = static_cast<Bound*>(event)->mFixed;
                        evPts.type = CurveRefPts::ePoint;
                        evPts.color = event->mColor;

                        dPts.Xmin = evPts.Xmin;//event->mTheta.mX; // always the same value
                        dPts.Xmax = evPts.Xmax;

                        dPts.Ymin = evPts.Ymin;
                        dPts.Ymax = evPts.Ymax;
                        dPts.color = event->mColor;
                        dPts.type = CurveRefPts::eRoundLine;

                        // memo Data Points
                        dataPts.append(dPts);
                        eventsPts.append(evPts);
                        hpdPerEvent.push_back(1);
                        dataPerEvent.push_back(1);
                    }
                }
            }
            // Fin de creation des points de ref
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

        const QStringList curveLongName = mModel->getCurvesLongName();
        const QString varRateText = tr("Var. Rate");
        const QString accelarationText = tr("Acceleration");

        const QString curveTitleX = curveLongName.at(0) ;

        if (mMainVariable == GraphViewResults::eGP) {
           graphX->setTitle(curveTitleX + " " + varRateText);


        } else if (mMainVariable == GraphViewResults::eGS) {
            graphX->setTitle(curveTitleX + " " + accelarationText);

         } else {
            graphX->setTitle(curveTitleX);
        }

        graphX->setComposanteG(modelCurve()->mPosteriorMeanG.gx);
        graphX->setComposanteGChains(modelCurve()->getChainsMeanGComposanteX());
        graphX->setEvents(modelCurve()->mEvents);


        if (mMainVariable == GraphViewResults::eG) {
            graphX->setEventsPoints(eventsPts);
            graphX->setDataPoints(dataPts);
        }

        mByCurveGraphs.append(graphX);

        connect(graphX, &GraphViewResults::selected, this, &ResultsView::updateOptionsWidget);

        
        if (displayY) {

            GraphViewCurve* graphY = new GraphViewCurve(widget);
            graphY->setSettings(mModel->mSettings);
            graphY->setMCMCSettings(mModel->mMCMCSettings, mModel->mChains);
            graphY->setGraphsFont(mFontBut->font());
            graphY->setGraphsThickness(mThicknessCombo->currentIndex());
            graphY->changeXScaleDivision(mMajorScale, mMinorCountScale);
            graphY->setMarginLeft(mMarginLeft);
            graphY->setMarginRight(mMarginRight);

            const QString curveTitleY = curveLongName.at(1) ;

            if (mMainVariable == GraphViewResults::eGP) {
               graphY->setTitle(curveTitleY + " " + varRateText);

            } else if (mMainVariable == GraphViewResults::eGS) {
                graphY->setTitle(curveTitleY + " " + accelarationText);

            } else {
                graphY->setTitle(curveTitleY);
            }

            graphY->setComposanteG(modelCurve()->mPosteriorMeanG.gy);
            graphY->setComposanteGChains(modelCurve()->getChainsMeanGComposanteY());

            graphY->setEvents(modelCurve()->mEvents);

            if (mMainVariable == GraphViewResults::eG) {
                // change the values of the Y and the error, with the values of the declination and the error, we keep tmean
                int i = 0;
                int iDataPts = 0;
                int iEventPts = -1;
                for (const auto& event : modelCurve()->mEvents) {
                    if (event->mIsSelected || showAllEvents) {
                        for (int j = 0 ; j< hpdPerEvent[i]; j++) {
                            iEventPts++;
                            if ( model->mCurveSettings.mProcessType == CurveSettings::eProcessType3D ||
                                 model->mCurveSettings.mProcessType == CurveSettings::eProcessType2D ) {
                                eventsPts[iEventPts].Ymin = event->mYDec - 1.96*event->mS_Y;
                                eventsPts[iEventPts].Ymax = event->mYDec + 1.96*event->mS_Y;

                            } else {
                                eventsPts[iEventPts].Ymin = event-> mYDec - event->mS_XA95Depth / cos(event->mXIncDepth * M_PI /180.);
                                eventsPts[iEventPts].Ymax = event-> mYDec + event->mS_XA95Depth / cos(event->mXIncDepth * M_PI /180.);
                            }

                        }

                        try {
                            for (int j = 0 ; j< dataPerEvent[i]; j++) {
                                dataPts[iDataPts].Ymin = eventsPts.at(iEventPts).Ymin;
                                dataPts[iDataPts].Ymax = eventsPts.at(iEventPts).Ymax;
                                iDataPts++;
                            }
                        } catch (...) {
                            qDebug() << "[ResultView::createByCurveGraph] pb graphY";
                        }

                        ++i;
                    }
                }
                graphY->setEventsPoints(eventsPts);
                graphY->setDataPoints(dataPts);
            }
            mByCurveGraphs.append(graphY);

            connect(graphY, &GraphViewResults::selected, this, &ResultsView::updateOptionsWidget);
        }
        
        if (displayZ) {
            GraphViewCurve* graphZ = new GraphViewCurve(widget);
            graphZ->setSettings(mModel->mSettings);
            graphZ->setMCMCSettings(mModel->mMCMCSettings, mModel->mChains);
            graphZ->setGraphsFont(mFontBut->font());
            graphZ->setGraphsThickness(mThicknessCombo->currentIndex());
            graphZ->changeXScaleDivision(mMajorScale, mMinorCountScale);
            graphZ->setMarginLeft(mMarginLeft);
            graphZ->setMarginRight(mMarginRight);

            const QString curveTitleZ = curveLongName.at(2) ;

            if (mMainVariable == GraphViewResults::eGP) {
               graphZ->setTitle(curveTitleZ + " " + varRateText);

            } else if (mMainVariable == GraphViewResults::eGS) {
                graphZ->setTitle(curveTitleZ + " " + accelarationText);

            } else {
                graphZ->setTitle(curveTitleZ);
            }


            graphZ->setComposanteG(modelCurve()->mPosteriorMeanG.gz);
            graphZ->setComposanteGChains(modelCurve()->getChainsMeanGComposanteZ());

            graphZ->setEvents(modelCurve()->mEvents);
            if (mMainVariable == GraphViewResults::eG) {
                int i = 0;
                int iDataPts = 0;
                int iEventPts = -1;
                for (const auto& event : modelCurve()->mEvents) {
                    if (event->mIsSelected || showAllEvents) {
                        //if (hpdPerEvent.size() > 1) {
                            for (int j = 0 ; j< hpdPerEvent[i]; j++) {
                                iEventPts++;
                                eventsPts[iEventPts].Ymin = event->mZField - 1.96*event->mS_ZField;
                                eventsPts[iEventPts].Ymax = event->mZField + 1.96*event->mS_ZField;
                            }
                       // }
                        for (int j =0 ; j< dataPerEvent[i]; j++) {
                            dataPts[iDataPts].Ymin = eventsPts.at(iEventPts).Ymin;
                            dataPts[iDataPts].Ymax = eventsPts.at(iEventPts).Ymax;
                            iDataPts++;
                        }
                        ++i;
                    }
                }
                graphZ->setEventsPoints(eventsPts);
                graphZ->setDataPoints(dataPts);
            }
            mByCurveGraphs.append(graphZ);

            connect(graphZ, &GraphViewResults::selected, this, &ResultsView::updateOptionsWidget);
        }
    }
}

void ResultsView::deleteAllGraphsInList(QList<GraphViewResults*>& list)
{
    for (auto&& graph : list) {
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
        byGraphs = mByCurveGraphs;
        break;
    default:
        byGraphs = QList<GraphViewResults*>();

    }

    if (onlySelected && !byGraphs.isEmpty()) {
        for (auto&& graph : byGraphs)
            if (graph->isSelected())
                graphs.append(graph);

        return graphs;

    } else {
        return byGraphs;
    }

}


#pragma mark Pagination


bool ResultsView::graphIndexIsInCurrentPage(int graphIndex)
{
    const int firstIndexToShow = mCurrentPage * mGraphsPerPage;
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
    QList<GraphViewResults*> listGraphs = currentGraphs(false);

    for (GraphViewResults*& graph : listGraphs) {
        graph->generateCurves(GraphViewResults::graph_t(mCurrentTypeGraph), mCurrentVariableList, mModel);
    }

   // std::for_each(PAR listGraphs.begin(), listGraphs.end(), [this] (GraphViewResults* g) {g->generateCurves(GraphViewResults::graph_t(mCurrentTypeGraph), mCurrentVariableList, mModel);} );

    updateCurvesToShow();
    updateGraphsMinMax();
    updateScales();
}

void ResultsView::updateGraphsMinMax()
{
    QList<GraphViewResults*> listGraphs = currentGraphs(false);
    if (mCurrentTypeGraph == GraphViewResults::ePostDistrib) {
        if (mMainVariable == GraphViewResults::eSigma) {
            mResultMinT = 0.;
            mResultMaxT = getGraphsMax(listGraphs, "Post Distrib", 100.);

        } else if (mMainVariable == GraphViewResults::eDuration) {
            mResultMinT = 0.;
            mResultMaxT = getGraphsMax(listGraphs, "Post Distrib All Chains", 100.);

        }else if (mMainVariable == GraphViewResults::eVg) {
            mResultMinT = 0;
            mResultMaxT = getGraphsMax(listGraphs, "Post Distrib All Chains", 100.);

        } else if (mMainVariable == GraphViewResults::eLambda) {
            mResultMinT = getGraphsMin(listGraphs, "Lambda", -20.);
            mResultMaxT = getGraphsMax(listGraphs, "Lambda", 10.);

        } else if (mMainVariable == GraphViewResults::eS02Vg) {
            mResultMinT = 0;
            mResultMaxT = getGraphsMax(listGraphs, "Post Distrib All Chains", 100.);

        } else {
            //auto span = (mModel->mSettings.getTmaxFormated() - mModel->mSettings.getTminFormated())/2. * mTimeSpin->minimum();
            mResultMinT = mModel->mSettings.getTminFormated();
            mResultMaxT = mModel->mSettings.getTmaxFormated();
        }

     } else if ((mCurrentTypeGraph == GraphViewResults::eTrace) || (mCurrentTypeGraph == GraphViewResults::eAccept)) {
            for (qsizetype i = 0; i<mChainRadios.size(); ++i) {
                if (mChainRadios.at(i)->isChecked()) {
                    const ChainSpecs& chain = mModel->mChains.at(i);
                    mResultMinT = 0;
                    const int adaptSize = chain.mBatchIndex * chain.mIterPerBatch;
                    const int runSize = chain.mRealyAccepted;
                    mResultMaxT = 1 + adaptSize + runSize;
                    break;
                }
            }

    } else if (mCurrentTypeGraph == GraphViewResults::eCorrel) {
        mResultMinT = 0.;
        mResultMaxT = 40;

    }
}

double ResultsView::getGraphsMax(const QList<GraphViewResults*>& graphs, const QString& title, double maxFloor)
{
    double max = 0.;

    for (const auto& graphWrapper : graphs) {
        const QList<GraphCurve> curves = graphWrapper->getGraph()->getCurves();
        for (const auto& curve : curves) {
              if (!curve.mData.isEmpty() && curve.mName.contains(title) && (curve.mVisible == true)) {
                max = ceil(std::max(max, curve.mData.lastKey()));
            }
        }
    }

    return std::max(maxFloor, max);
}

double ResultsView::getGraphsMin(const QList<GraphViewResults*>& graphs, const QString& title, double minFloor)
{
    double min = 0.;

    for (const auto& graphWrapper : graphs) {
        const QList<GraphCurve> curves = graphWrapper->getGraph()->getCurves();
        for (const auto& curve : curves) {
            if (!curve.mData.isEmpty() && curve.mName.contains(title) && (curve.mVisible == true)) {
                min = floor(std::min(min, curve.mData.firstKey()));
            }
        }
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
    const bool showAllChains = mAllChainsCheck->isChecked();

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
    QVector<GraphViewResults::variable_t> showVariableList;
    // --------------------------------------------------------
    //  Options for "Curves"
    // --------------------------------------------------------
    if ((mGraphListTab->currentName() == tr("Curves")) && !mLambdaRadio->isChecked() && !mS02VgRadio->isChecked()) {

       if (mCurveGRadio->isChecked()) showVariableList.append(GraphViewResults::eG);
       if (mCurveErrorCheck->isChecked()) showVariableList.append(GraphViewResults::eGError);
       if (mCurveMapCheck->isChecked()) showVariableList.append(GraphViewResults::eMap);
       if (mCurveEventsPointsCheck->isChecked()) showVariableList.append(GraphViewResults::eGEventsPts);
       if (mCurveDataPointsCheck->isChecked()) showVariableList.append(GraphViewResults::eGDatesPts);

       if (mCurveGPRadio->isChecked()) showVariableList.append(GraphViewResults::eGP);
       if (mCurveGSRadio->isChecked()) showVariableList.append(GraphViewResults::eGS);

       const bool showStat = mCurveStatCheck->isChecked();

        // --------------------------------------------------------
        //  Update Graphs with selected options
        // --------------------------------------------------------
        for (GraphViewResults*& graph : listGraphs) {

            GraphViewCurve* graphCurve = static_cast<GraphViewCurve*>(graph);
            graphCurve->setShowNumericalResults(showStat);

            QString graphName = mModel->getCurvesLongName().at(0);
            const QString varRateText = tr("Var. Rate");
            const QString accelarationText = tr("Acceleration");

            if (mCurveGPRadio->isChecked())
                graphName = graphName + " " + varRateText;
            else if (mCurveGSRadio->isChecked())
                graphName = graphName + " " + accelarationText;

            if (graph->title() == graphName) {
                Scale scaleX;
                scaleX.findOptimalMark(mResultCurrentMinX, mResultCurrentMaxX, 10);
                graphCurve->updateCurvesToShowForG(showAllChains, showChainList, showVariableList, scaleX);
            }

            if (mModel->displayY() ) {
                graphName = mModel->getCurvesLongName().at(1);
                if (mCurveGPRadio->isChecked())
                    graphName = graphName + " " + varRateText;
                else if (mCurveGSRadio->isChecked())
                    graphName = graphName + " " + accelarationText;

                if (graph->title() == graphName) {
                    Scale scaleY;
                    scaleY.findOptimalMark(mResultCurrentMinY, mResultCurrentMaxY, 10);
                    graphCurve->updateCurvesToShowForG(showAllChains, showChainList, showVariableList, scaleY);
                }

                if (mModel->displayZ()) {
                    graphName = mModel->getCurvesLongName().at(2);
                    if (mCurveGPRadio->isChecked())
                        graphName = graphName + " " + varRateText;
                    else if (mCurveGSRadio->isChecked())
                        graphName = graphName + " " + accelarationText;

                    if (graph->title() == graphName) {
                        Scale scaleZ;
                        scaleZ.findOptimalMark(mResultCurrentMinZ, mResultCurrentMaxZ, 10);
                        graphCurve->updateCurvesToShowForG(showAllChains, showChainList, showVariableList, scaleZ);
                    }

                }
            }
            graphCurve->update();

        }
        return;

    } else if ((mGraphListTab->currentName() == tr("Curves")) && mLambdaRadio->isChecked() && !mS02VgRadio->isChecked()) {
        if (mCredibilityCheck->isChecked())
            showVariableList.append(GraphViewResults::eCredibility);
        showVariableList.append(GraphViewResults::eLambda);

    } else if ((mGraphListTab->currentName() == tr("Curves")) && !mLambdaRadio->isChecked() && mS02VgRadio->isChecked()) {
        if (mCredibilityCheck->isChecked())
            showVariableList.append(GraphViewResults::eCredibility);
        showVariableList.append(GraphViewResults::eS02Vg);

    } else if (mGraphListTab->currentName() == tr("Events")) {
        if (mCredibilityCheck->isChecked())
            showVariableList.append(GraphViewResults::eCredibility);
        if (mEventThetaRadio->isChecked()) {
            showVariableList.append(GraphViewResults::eThetaEvent);
            if (mEventsDatesUnfoldCheck->isChecked()) {
               showVariableList.append(GraphViewResults::eDataTi);
               if (mDataCalibCheck->isChecked()) showVariableList.append(GraphViewResults::eDataCalibrate);
               if (mWiggleCheck->isChecked()) showVariableList.append(GraphViewResults::eDataWiggle);

            }
        } else if (mDataSigmaRadio->isChecked()) {
            if (mCredibilityCheck->isChecked())
                showVariableList.append(GraphViewResults::eCredibility);
            showVariableList.append(GraphViewResults::eSigma);

        } else if (mEventVGRadio->isChecked()) {
            if (mCredibilityCheck->isChecked())
                showVariableList.append(GraphViewResults::eCredibility);
            showVariableList.append(GraphViewResults::eVg);
        }

    }
    else if (mGraphListTab->currentName() == tr("Phases")) {
        if (mBeginEndRadio->isChecked()) {
               showVariableList.append(GraphViewResults::eBeginEnd);
               if (mCredibilityCheck->isChecked())
                   showVariableList.append(GraphViewResults::eCredibility);
               if (mPhasesEventsUnfoldCheck->isChecked()) {
                   showVariableList.append(GraphViewResults::eThetaEvent);
                   if (mPhasesDatesUnfoldCheck->isChecked()) {
                       showVariableList.append(GraphViewResults::eDataTi);
                   }
               }
        } else if (mTempoRadio->isChecked()) {
            showVariableList.append(GraphViewResults::eTempo);
            if (mErrCheck->isChecked())
               showVariableList.append(GraphViewResults::eError);


            if (mPhasesEventsUnfoldCheck->isChecked()) {
                if (mCredibilityCheck->isChecked())
                    showVariableList.append(GraphViewResults::eCredibility);
                showVariableList.append(GraphViewResults::eThetaEvent);
                if (mPhasesDatesUnfoldCheck->isChecked()) {
                    showVariableList.append(GraphViewResults::eDataTi);


                }
            }

        } else if (mActivityRadio->isChecked()) {
            showVariableList.append(GraphViewResults::eActivity);
            if (mErrCheck->isChecked())
               showVariableList.append(GraphViewResults::eError);
            if (mActivityUnifCheck->isChecked())
               showVariableList.append(GraphViewResults::eActivityUnif);

            if (mPhasesEventsUnfoldCheck->isChecked()) {
                if (mCredibilityCheck->isChecked())
                    showVariableList.append(GraphViewResults::eCredibility);
                showVariableList.append(GraphViewResults::eThetaEvent);
                if (mPhasesDatesUnfoldCheck->isChecked()) {
                    showVariableList.append(GraphViewResults::eDataTi);


                }
            }

        } else if (mDurationRadio->isChecked()) {
            if (mCredibilityCheck->isChecked())
                showVariableList.append(GraphViewResults::eCredibility);
            showVariableList.append(GraphViewResults::eDuration);
        }
    }
    // --------------------------------------------------------
    //  All others
    // --------------------------------------------------------
    else { // it's curves !!

    }
    const bool showStat = mStatCheck->isChecked();
    // --------------------------------------------------------
    //  Update Graphs with selected options
    // --------------------------------------------------------
    for (GraphViewResults*& graph : listGraphs) {
        graph->setShowNumericalResults(showStat);
        graph->updateCurvesToShow(showAllChains, showChainList, showVariableList);
    }

}

/**
 *  @brief
 *  This method does the following :
 *  - Defines [mResultMinT, mResultMaxT]
 *  - Defines [mResultCurrentMinT, mResultCurrentMaxT] (based on saved zoom if any)
 *  - Computes mResultZoomT
 *  - Set Ruler Areas
 *  - Set Ruler and graphs range and zoom
 *  - Update mXMinEdit, mXMaxEdit, mXSlider, mTimeSpin, mMajorScaleEdit, mMinorScaleEdit
 */
void ResultsView::updateScales()
{
    if (!mModel) {
        return;
    }

    const StudyPeriodSettings s = mModel->mSettings;

    // ------------------------------------------------------------------
    //  Define mResultCurrentMinT and mResultCurrentMaxT
    //  + Restore last zoom values if any
    // ------------------------------------------------------------------

    // The key of the saved zooms map is as long as that :
    QPair<GraphViewResults::variable_t, GraphViewResults::graph_t> key(mMainVariable, mCurrentTypeGraph);

    // Anyway, let's check if we have a saved zoom value for this key :
    if (mZooms.find(key) != mZooms.end()) {
        // Get the saved (unformatted) values
        const double tMin = mZooms.value(key).first;
        const double tMax = mZooms.value(key).second;

        // These graphs needs formating since the x axis represents the time
        if (xScaleRepresentsTime()) {
            double tMinFormatted = DateUtils::convertToAppSettingsFormat(tMin);
            double tMaxFormatted = DateUtils::convertToAppSettingsFormat(tMax);

            // Min and max may be inverted due to formatting, so we use std::minmax
            std::pair<double, double> currentMinMax = std::minmax(tMinFormatted, tMaxFormatted);

            mResultCurrentMinT = currentMinMax.first;
            mResultCurrentMaxT = currentMinMax.second;

        } else {
            mResultCurrentMinT = tMin;
            mResultCurrentMaxT = tMax;
        }

    } else {
        if ((mCurrentTypeGraph == GraphViewResults::eTrace) || (mCurrentTypeGraph == GraphViewResults::eAccept)) {
            for (int i = 0; i<mChainRadios.size(); ++i) {
                if (mChainRadios.at(i)->isChecked()) {
                    const ChainSpecs& chain = mModel->mChains.at(i);
                    mResultCurrentMinT = 0;
                    const int adaptSize = chain.mBatchIndex * chain.mIterPerBatch;
                    const int runSize = chain.mRealyAccepted;
                    mResultCurrentMaxT = 1 + chain.mIterPerBurn + adaptSize + runSize;
                    break;
                }
            }
        } else if (mCurrentTypeGraph == GraphViewResults::eCorrel) {
            mResultCurrentMinT = 0;
            mResultCurrentMaxT = 40;

        } else {
            mResultCurrentMinT = mResultMinT;
            mResultCurrentMaxT = mResultMaxT;
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
            xScale.findOptimalMark(mResultCurrentMinT, mResultCurrentMaxT, 10);
            mMajorScale = xScale.mark;
            mMinorCountScale = 10;
        }
    }

    // ------------------------------------------------------------------
    //  Compute mResultZoomT
    // ------------------------------------------------------------------
    mResultZoomT = (mResultMaxT - mResultMinT) / (mResultCurrentMaxT - mResultCurrentMinT);

    // ------------------------------------------------------------------
    //  Set Ruler Areas (Burn, Adapt, Run)
    // ------------------------------------------------------------------
    mRuler->clearAreas();

    if ( xScaleRepresentsTime() ) {
        // The X zoom uses a log scale on the spin box and can be controlled by the linear slider
         mXSlider->setRange(-100, 100);
         mTimeSpin->setRange(sliderToZoom(-100), sliderToZoom(100));
         mTimeSpin->setSingleStep(.01);
         mTimeSpin->setDecimals(3);

         // The Ruler range is much wider based on the minimal zoom
         const double tCenter = (mResultCurrentMinT + mResultCurrentMaxT) / 2.;
         const double tSpan = mResultCurrentMaxT - mResultCurrentMinT;
         const double tRangeMin = tCenter - ((tSpan/2.) / sliderToZoom(mXSlider->minimum()));
         const double tRangeMax = tCenter + ((tSpan/2.) / sliderToZoom(mXSlider->minimum()));

         mRuler->setRange(tRangeMin, tRangeMax);
         mRuler->setFormatFunctX(nullptr);

    } else if (mCurrentTypeGraph == GraphViewResults::ePostDistrib &&
               ( mMainVariable == GraphViewResults::eSigma ||
                 mMainVariable == GraphViewResults::eVg ||
                 mMainVariable == GraphViewResults::eDuration ||
                 mMainVariable == GraphViewResults::eS02Vg ) ) {

                // The X zoom uses a log scale on the spin box and can be controlled by the linear slider
                mXSlider->setRange(-100, 100);
                mTimeSpin->setRange(sliderToZoom(-100), sliderToZoom(100));
                mTimeSpin->setSingleStep(.01);
                mTimeSpin->setDecimals(3);

                // The Ruler range is much wider based on the minimal zoom
                const double tRangeMax = mResultMaxT / sliderToZoom(mXSlider->minimum());

                mRuler->setRange(0, tRangeMax);
                mRuler->setFormatFunctX(nullptr);



    } else if (mCurrentTypeGraph == GraphViewResults::ePostDistrib &&
               mMainVariable == GraphViewResults::eLambda ) {

                // The X zoom uses a log scale on the spin box and can be controlled by the linear slider
                mXSlider->setRange(-100, 100);
                mTimeSpin->setRange(sliderToZoom(-100), sliderToZoom(100));
                mTimeSpin->setSingleStep(.01);
                mTimeSpin->setDecimals(3);

                // The Ruler range is much wider based on the minimal zoom
                const double tCenter = (mResultCurrentMinT + mResultCurrentMaxT) / 2.;
                const double tSpan = mResultCurrentMaxT - mResultCurrentMinT;
                const double tRangeMin = tCenter - ((tSpan/2.) / sliderToZoom(mXSlider->minimum()));
                const double tRangeMax = tCenter + ((tSpan/2.) / sliderToZoom(mXSlider->minimum()));

                mRuler->setRange(tRangeMin, tRangeMax);
                mRuler->setFormatFunctX(nullptr);

    } else if ( mCurrentTypeGraph == GraphViewResults::eTrace ||
                mCurrentTypeGraph == GraphViewResults::eAccept) {
        int idSelect = 0;
        for (const auto& chRadio : mChainRadios) {
            if (chRadio->isChecked())
                break;
            ++idSelect;
        }

        const ChainSpecs& chain = mModel->mChains.at(idSelect);
        const int adaptSize = chain.mBatchIndex * chain.mIterPerBatch;
        const int runSize = chain.mRealyAccepted;
        mResultMaxT = 1 +  chain.mIterPerBurn + adaptSize + runSize;
        // The min is always 0
        mResultMinT = 0.;

        mRuler->addArea(0., 1+ chain.mIterPerBurn, QColor(235, 115, 100));
        mRuler->addArea(1+ chain.mIterPerBurn, 1+chain.mIterPerBurn + adaptSize, QColor(250, 180, 90));
        mRuler->addArea(1+ chain.mIterPerBurn + adaptSize, mResultMaxT, QColor(130, 205, 110));
        // The Ruler range is set exactly to the min and max (impossible to scroll outside)
        mRuler->setRange(mResultMinT, mResultMaxT);
        mRuler->setFormatFunctX(nullptr);


        // The zoom slider and spin are linear.
        // The number of zoom levels depends on the number of iterations (by a factor 100)
        // e.g. 400 iterations => 4 levels
        const int zoomLevels = (int) mResultMaxT / 100;
        mXSlider->setRange(1, zoomLevels);
        mTimeSpin->setRange(1, zoomLevels);
        mTimeSpin->setSingleStep(1.);
        mTimeSpin->setDecimals(0);

        // find new minY and maxY


    } else if ( mCurrentTypeGraph == GraphViewResults::eCorrel) {
        // The x axis represents h, always in [0, 40]
        mResultMinT = 0.;
        mResultMaxT = 40.;

        // The zoom slider and spin are linear.
        // Always 5 zoom levels
        mXSlider->setRange(1, 5);
        mTimeSpin->setRange(1, 5);
        mTimeSpin->setSingleStep(1.);
        mTimeSpin->setDecimals(0);

        mRuler->setRange(mResultMinT, mResultMaxT);
        mRuler->setFormatFunctX(nullptr);
    }


    // ------------------------------------------------------------------
    //  Apply to Ruler
    // ------------------------------------------------------------------
    mRuler->setCurrent(mResultCurrentMinT, mResultCurrentMaxT);
    mRuler->setScaleDivision(mMajorScale, mMinorCountScale);

    // -------------------------------------------------------
    //  Apply to all graphs
    // -------------------------------------------------------
    QList<GraphViewResults*> graphs = currentGraphs(false);
    for (GraphViewResults*& graph : graphs) {
        graph->setRange(mRuler->mMin, mRuler->mMax);
        graph->setCurrentX(mResultCurrentMinT, mResultCurrentMaxT);
        graph->changeXScaleDivision(mMajorScale, mMinorCountScale);
        graph->zoom(mResultCurrentMinT, mResultCurrentMaxT);
    }

    // -------------------------------------------------------
    //  Set options UI components values
    // -------------------------------------------------------
    setTimeRange();
    setTimeSlider(zoomToSlider(mResultZoomT));
    setTimeSpin(mResultZoomT);
    setTimeScale();

    // -------------------------------------------------------
    // X option
    // -------------------------------------------------------
    if (mModel->displayX()) {
        if (mZoomsX.find(key) != mZoomsX.end()) {
            const double XMin = mZoomsX.value(key).first;
            const double XMax = mZoomsX.value(key).second;

            mResultCurrentMinX = XMin;
            mResultCurrentMaxX = XMax;
            setXRange();
            updateCurvesToShow();

        } else {
            findOptimalX();
        }
    }
    // -------------------------------------------------------
    // Y option
    // -------------------------------------------------------
    if (mModel->displayY()) {
        if (mZoomsY.find(key) != mZoomsY.end()) {
            const double YMin = mZoomsY.value(key).first;
            const double YMax = mZoomsY.value(key).second;

            mResultCurrentMinY = YMin;
            mResultCurrentMaxY = YMax;
            setYRange();
            updateCurvesToShow();

        } else {
            findOptimalY();
        }
        // -------------------------------------------------------
        // Z option
        // -------------------------------------------------------
        if (mModel->displayZ()) {
            if (mZoomsZ.find(key) != mZoomsZ.end()) {
                const double ZMin = mZoomsZ.value(key).first;
                const double ZMax = mZoomsZ.value(key).second;

                mResultCurrentMinZ = ZMin;
                mResultCurrentMaxZ = ZMax;
                setZRange();
                updateCurvesToShow();

            } else {
                findOptimalZ();
            }
        }
    }

}


void ResultsView::updateOptionsWidget()
{
    const bool isPostDistrib = isPostDistribGraph();
    unsigned optionWidgetHeigth = 0;
    // -------------------------------------------------------------------------------------
    //  Update graph list tab
    // -------------------------------------------------------------------------------------

    mGraphListTab->setTabVisible(1, mHasPhases); // Phases
    mGraphListTab->setTabVisible(2, isCurve()); // Curve


    delete mOptionsLayout;
    mOptionsLayout = new QVBoxLayout();
    mOptionsLayout->setContentsMargins(mMargin, mMargin, 0, 0);
    mOptionsLayout->addWidget(mGraphListTab);
    // If the current tab is not currently visible :
    // - Show the "Phases" tab (1) which is a good default choice if the model has phases.
    // - Show the "Events" tab (0) which is a good default choice if the model doesn't have phases.

    if (mHasPhases && mGraphListTab->currentIndex() >= 2 && !isCurve()) {
        mGraphListTab->setTab(1, false);

    } else if (!mHasPhases && !isCurve() && mGraphListTab->currentIndex() >= 1) {
        mGraphListTab->setTab(0, false);
    }


    optionWidgetHeigth += mGraphListTab->height();
    // -------------------------------------------------------------------------------------
    //  Update controls depending on current graph list
    // -------------------------------------------------------------------------------------
    if (mGraphListTab->currentName() == tr("Events")) {
        mOptionsLayout->addWidget(mEventsGroup);

        mGraphTypeTabs->setTabVisible(1, true); // History Plot
        mGraphTypeTabs->setTabVisible(2, true); // Acceptance Rate
        mGraphTypeTabs->setTabVisible(3, true); // Autocorrelation

        mEventsGroup->show();
        mPhasesGroup->setVisible(false);
        mCurvesGroup->setVisible(false);
        const qreal h = mEventThetaRadio->height() * 1.5;

        mEventVGRadio->setVisible(isCurve());
        //--- change layout


        QVBoxLayout* eventGroupLayout = new QVBoxLayout();
        eventGroupLayout->setContentsMargins(10, 10, 10, 10);
        //eventGroupLayout->setSpacing(15);
        eventGroupLayout->addWidget(mEventThetaRadio);
        eventGroupLayout->addWidget(mDataSigmaRadio);
        qreal totalH =  3*h;
        if (isCurve()) {
            mEventVGRadio->show();
            eventGroupLayout->addWidget(mEventVGRadio);
            totalH += h;
        } else {
            mEventVGRadio->hide();
        }

        eventGroupLayout->addWidget(mEventsDatesUnfoldCheck);

        if (isPostDistrib && mEventThetaRadio->isChecked() && mEventsDatesUnfoldCheck->isChecked()) {
            mDataCalibCheck->show();
            mWiggleCheck->show();
            totalH += 2 * h;

            QVBoxLayout* unFoldGroupLayout = new QVBoxLayout();
            unFoldGroupLayout->setContentsMargins(15, 0, 0, 0);
            unFoldGroupLayout->addWidget(mDataCalibCheck, Qt::AlignLeft);
            unFoldGroupLayout->addWidget(mWiggleCheck, Qt::AlignLeft);

            eventGroupLayout->addLayout(unFoldGroupLayout);

        } else {
            mDataCalibCheck->hide();
            mWiggleCheck->hide();
        }
        eventGroupLayout->addWidget(mStatCheck);
        totalH += h;

        delete mEventsGroup->layout() ;
        mEventsGroup->setLayout(eventGroupLayout);

        //-- end new layout

        mEventsGroup->setFixedHeight(totalH);

        optionWidgetHeigth += mEventsGroup->height();

    } else if (mGraphListTab->currentName() == tr("Phases")) { // phases tab
         mOptionsLayout->addWidget(mPhasesGroup);

        mGraphTypeTabs->setTabVisible(1, true); // History Plot
        mGraphTypeTabs->setTabVisible(2, true); // Acceptance Rate
        mGraphTypeTabs->setTabVisible(3, true); // Autocorrelation

        mEventsGroup->setVisible(false);
        mPhasesGroup->setVisible(true);
        mCurvesGroup->setVisible(false);

        const qreal h = mPhasesEventsUnfoldCheck->height() * 1.5;

        if (!mPhasesEventsUnfoldCheck->isChecked()) {
            mPhasesDatesUnfoldCheck->setChecked(false);
        }

        //--- Change layout
        QVBoxLayout* phasesGroupLayout = new QVBoxLayout();
        phasesGroupLayout->setContentsMargins(10, 10, 10, 10);

        phasesGroupLayout->addWidget(mBeginEndRadio);
        phasesGroupLayout->addWidget(mTempoRadio);
        phasesGroupLayout->addWidget(mActivityRadio);
        qreal totalH = 3 * h; // look ligne 165 in ResultsView() comment Right Part. totalH = 3 * h


        QVBoxLayout* phasesUnfoldErrorGroupLayout = new QVBoxLayout();
        phasesUnfoldErrorGroupLayout->setContentsMargins(15, 0, 0, 0);

        if (mBeginEndRadio->isChecked() || mDurationRadio->isChecked()) {

           mErrCheck->hide();
           mActivityUnifCheck->hide();

        } else if (mTempoRadio->isChecked()) {

            mErrCheck->show();
            phasesUnfoldErrorGroupLayout->addWidget(mErrCheck, Qt::AlignLeft);
            totalH += h;
            mActivityUnifCheck->hide();

        } else if (mActivityRadio->isChecked()) {

            mErrCheck->show();
            phasesUnfoldErrorGroupLayout->addWidget(mErrCheck, Qt::AlignLeft);
            totalH += h;
            mActivityUnifCheck->show();
            phasesUnfoldErrorGroupLayout->addWidget(mActivityUnifCheck, Qt::AlignLeft);
            totalH += h;
        }
        phasesGroupLayout->addWidget(mDurationRadio);

        mPhasesEventsUnfoldCheck->show();
        phasesUnfoldErrorGroupLayout->addWidget(mPhasesEventsUnfoldCheck, Qt::AlignLeft);
        totalH += h;
        if (mPhasesEventsUnfoldCheck->isChecked()) {
            mPhasesDatesUnfoldCheck->show();
            phasesUnfoldErrorGroupLayout->addWidget(mPhasesDatesUnfoldCheck, Qt::AlignLeft);
            totalH += h;

        } else {
            mPhasesDatesUnfoldCheck->hide();
        }

        phasesGroupLayout->addLayout(phasesUnfoldErrorGroupLayout);


        phasesGroupLayout->addWidget(mPhasesStatCheck);
        totalH += 2*h;

        delete mPhasesGroup->layout() ;
        mPhasesGroup->setLayout(phasesGroupLayout);
        //-- end new layout

        mPhasesGroup->setFixedHeight(totalH);
        optionWidgetHeigth += mPhasesGroup->height();

    } else if (mGraphListTab->currentName() == tr("Curves")) { // curve Tab
        mOptionsLayout->addWidget(mCurvesGroup);

        const bool isLambda = mLambdaRadio->isChecked();
        const bool isS02Vg = mS02VgRadio->isChecked();

        mGraphTypeTabs->setTabVisible(1, isLambda || isS02Vg); // History Plot
        mGraphTypeTabs->setTabVisible(2, isLambda || isS02Vg); // Acceptance Rate
        mGraphTypeTabs->setTabVisible(3, isLambda || isS02Vg); // Autocorrelation

        mEventsGroup->setVisible(false);
        mPhasesGroup->setVisible(false);
        mCurvesGroup->setVisible(true);
        const qreal h = mCurveGRadio->height() * 1.3;

        //--- change layout
        QVBoxLayout* curveGroupLayout = new QVBoxLayout();

        curveGroupLayout->setContentsMargins(10, 10, 10, 10);
        curveGroupLayout->addWidget(mCurveGRadio);
        qreal totalH =  h;

        if (mCurveGRadio->isChecked()) {
            mCurveErrorCheck->show();
            mCurveMapCheck->show();
            mCurveEventsPointsCheck->show();
            mCurveDataPointsCheck->show();
            totalH += 4 * mCurveErrorCheck->height()*1.3 ;

            QVBoxLayout* curveOptionGroupLayout = new QVBoxLayout();
            curveOptionGroupLayout->setContentsMargins(15, 0, 0, 0);
            curveOptionGroupLayout->addWidget(mCurveErrorCheck, Qt::AlignLeft);
            curveOptionGroupLayout->addWidget(mCurveMapCheck, Qt:: AlignLeft);
            curveOptionGroupLayout->addWidget(mCurveEventsPointsCheck, Qt::AlignLeft);
            curveOptionGroupLayout->addWidget(mCurveDataPointsCheck, Qt::AlignLeft);

            curveGroupLayout->addLayout(curveOptionGroupLayout);

        } else {
            mCurveErrorCheck->hide();
            mCurveMapCheck->hide();
            mCurveEventsPointsCheck->hide();
            mCurveDataPointsCheck->hide();
        }
        curveGroupLayout->addWidget(mCurveGPRadio);
        curveGroupLayout->addWidget(mCurveGSRadio);
        curveGroupLayout->addWidget(mLambdaRadio);
        curveGroupLayout->addWidget(mS02VgRadio);
        curveGroupLayout->addWidget(mCurveStatCheck);
        totalH += 5 * h;

        delete mCurvesGroup->layout() ;
        mCurvesGroup->setLayout(curveGroupLayout);

        //-- end new layout

        mCurvesGroup->setFixedHeight(totalH);
        optionWidgetHeigth += mCurvesGroup->height();
    }

    // ------------------------------------
    //  Display / Distrib. Option
    // ------------------------------------
    optionWidgetHeigth += mDisplayDistribTab->height();

    mOptionsLayout->addWidget(mDisplayDistribTab);

    qreal widHeigth = 0;
    const  qreal internSpan = 10;
    if (mDisplayDistribTab->currentName() == tr("Display") ) {
        mDisplayWidget->show();
        mDistribWidget->hide();
        mOptionsLayout->addWidget(mDisplayWidget);

        // ------------------------------------
        //  Display Options
        // ------------------------------------

        const qreal h = mDisplayStudyBut->height();

        widHeigth = 11*h + 13*internSpan;
        /* 11*h = spanOptionTitle + studyPeriodButton + span + slider + majorInterval + minorCount
         *        + GraphicOptionsTitle + ZoomSlider + Font + Thickness + Opacity
         */


        mDisplayStudyBut->setText(xScaleRepresentsTime() ? tr("Study Period Display") : tr("Fit Display"));
        mDisplayStudyBut->setVisible(true);
        widHeigth += mDisplayStudyBut->height() + internSpan;

        const bool displayX = mModel ? mModel->displayX() : false;
        const bool displayY = mModel ? mModel->displayY() : false;
        const bool displayZ = mModel ? mModel->displayZ() : false;

        if (isCurve() && ( mMainVariable == GraphViewResults::eG ||
                           mMainVariable == GraphViewResults::eGP ||
                           mMainVariable == GraphViewResults::eGS)) {
            mXOptionTitle->setVisible(displayX);
            mXOptionGroup->setVisible(displayX);
            widHeigth += displayX ? 2*h + 2*internSpan : 0.;// mXOptionGroup->height() + mXOptionTitle->height() + internSpan: 0.;

            mYOptionTitle->setVisible(displayY);
            mYOptionGroup->setVisible(displayY);
            widHeigth += displayY ? 2*h + 2*internSpan : 0.;//mYOptionGroup->height() + mYOptionTitle->height() + internSpan: 0.;

            mZOptionTitle->setVisible(displayZ);
            mZOptionGroup->setVisible(displayZ);
            widHeigth += displayZ ? 2*h + 3*internSpan : 0.;//mZOptionGroup->height() + mZOptionTitle->height() + internSpan: 0.;
            mDisplayWidget->setFixedHeight(widHeigth);

        } else {
            mXOptionTitle->setVisible(false);
            mXOptionGroup->setVisible(false);

            mYOptionTitle->setVisible(false);
            mYOptionGroup->setVisible(false);

            mZOptionTitle->setVisible(false);
            mZOptionGroup->setVisible(false);
            mDisplayWidget-> setFixedHeight(widHeigth);
        }

        optionWidgetHeigth += widHeigth; //mDisplayWidget->height();

    } else {
        mDisplayWidget->hide();
        mDistribWidget->show();
        // ------------------------------------
        //  MCMC Chains
        //  Switch between checkBoxes or Radio-buttons for chains
        // ------------------------------------
        mAllChainsCheck->setVisible(isPostDistrib);

        if (isPostDistrib)
            widHeigth += internSpan + mAllChainsCheck->height() + internSpan;

        for (auto&& checkChain : mChainChecks) {
            checkChain->setVisible(isPostDistrib);
            if (isPostDistrib)
                widHeigth += checkChain->height() + internSpan;
        }

        for (auto&& chainRadio : mChainRadios) {
            chainRadio->setVisible(!isPostDistrib);
            if (!isPostDistrib)
                widHeigth += chainRadio->height() + internSpan;
        }

        mChainsGroup->setFixedHeight(widHeigth);
        widHeigth += mChainsTitle->height();
        // ------------------------------------
        //  Density Options
        // ------------------------------------
        bool showDensityOptions = isPostDistrib &&
                (  mMainVariable != GraphViewResults::eG
                && mMainVariable != GraphViewResults::eGP
                && mMainVariable != GraphViewResults::eGS);


        mFFTLenLab->show();
        mFFTLenLab->setFixedHeight(20);
        mFFTLenCombo->show();
        mFFTLenCombo->setFixedHeight(20);
        int nbObject = 0;

        if (mMainVariable == GraphViewResults::eActivity) {
            mRangeThreshLab->show();
            mRangeThreshLab->setFixedHeight(20);
            mRangeThresholdEdit->show();
            mRangeThresholdEdit->setFixedHeight(20);

            mThreshLab->show();
            mThreshLab->setFixedHeight(20);
            mThresholdEdit->show();
            mThresholdEdit->setFixedHeight(20);

            mCredibilityCheck->hide();
            mCredibilityCheck->setFixedHeight(0);

            mBandwidthLab->hide();
            mBandwidthLab->setFixedHeight(0);
            mBandwidthSpin->hide();
            mBandwidthSpin->setFixedHeight(0);

            mHActivityLab->show();
            mHActivityLab->setFixedHeight(20);
            mHActivityEdit->show();
            mHActivityEdit->setFixedHeight(20);

            nbObject += 3;

        } else if (mMainVariable == GraphViewResults::eTempo) {
            mRangeThreshLab->hide();
            mRangeThreshLab->setFixedHeight(0);
            mRangeThresholdEdit->hide();
            mRangeThresholdEdit->setFixedHeight(0);

            mThreshLab->hide();
            mThreshLab->setFixedHeight(0);
            mThresholdEdit->hide();
            mThresholdEdit->setFixedHeight(0);

            mCredibilityCheck->hide();
            mCredibilityCheck->setFixedHeight(0);

            mBandwidthLab->hide();
            mBandwidthLab->setFixedHeight(0);
            mBandwidthSpin->hide();
            mBandwidthSpin->setFixedHeight(0);

            mHActivityLab->hide();
            mHActivityLab->setFixedHeight(0);
            mHActivityEdit->hide();
            mHActivityEdit->setFixedHeight(0);

            //nbObject += 0;

        } else {
            mRangeThreshLab->hide();
            mRangeThreshLab->setFixedHeight(0);
            mRangeThresholdEdit->hide();
            mRangeThresholdEdit->setFixedHeight(0);

            mThreshLab->show();
            mThreshLab->setFixedHeight(20);
            mThresholdEdit->show();
            mThresholdEdit->setFixedHeight(20);

            mCredibilityCheck->show();
            mCredibilityCheck->setFixedHeight(20);

            mBandwidthLab->show();
            mBandwidthLab->setFixedHeight(20);
            mBandwidthSpin->show();
            mBandwidthSpin->setFixedHeight(20);

            mHActivityLab->hide();
            mHActivityLab->setFixedHeight(0);
            mHActivityEdit->hide();
            mHActivityEdit->setFixedHeight(0);

            nbObject += 3;
        }

        mDensityOptsTitle->setVisible(showDensityOptions);
        mDensityOptsGroup->setVisible(showDensityOptions);

        if (showDensityOptions) {

            mDensityOptsGroup->setFixedHeight( mCredibilityCheck->height() + mThresholdEdit->height() + mFFTLenCombo->height()
                                               + mBandwidthSpin->height() + mHActivityEdit->height() + mRangeThresholdEdit->height()
                                               + (nbObject+1)* internSpan);

            widHeigth += mDensityOptsTitle->height() + mDensityOptsGroup->height() + 4*internSpan;

        } else
            widHeigth += 2*internSpan;

        mDistribWidget->setFixedHeight(widHeigth);
        mOptionsLayout->addWidget(mDistribWidget);
        optionWidgetHeigth += mDistribWidget->height();
    }

    optionWidgetHeigth += 35;//40; // ???

    // -------------------------------------------------------------------------------------
    //  Page / Save
    // -------------------------------------------------------------------------------------
    mOptionsLayout->addWidget(mPageSaveTab);
    optionWidgetHeigth += mPageSaveTab->height();

    if (mPageSaveTab->currentName() == tr("Page") ) {

        // -------------------------------------------------------------------------------------
        //  - Update the total number of graphs for all pages
        //  - Check if the current page is still lower than the number of pages
        //  - Update the pagination display
        //  => All this must be done BEFORE calling createGraphs, which uses theses params to build the graphs
        // -------------------------------------------------------------------------------------
        updateTotalGraphs();

        const int numPages = ceil((double)mMaximunNumberOfVisibleGraph / (double)mGraphsPerPage);
        if (mCurrentPage >= numPages) {
            mCurrentPage = 0;
        }

        mPageEdit->setText(locale().toString(mCurrentPage + 1) + "/" + locale().toString(numPages));

        mPageWidget->setVisible(true);
        mSaveSelectWidget->hide();
        mSaveAllWidget->hide();


        mOptionsLayout->addWidget(mPageWidget);
        optionWidgetHeigth += mPageWidget->height();

    } else  if (currentGraphs(true).isEmpty()) {
        mPageWidget->hide();
        mSaveSelectWidget->hide();
        mSaveAllWidget->show();
        mOptionsLayout->addWidget(mSaveAllWidget);
        optionWidgetHeigth += mSaveAllWidget->height();

   } else {
        mPageWidget->hide();
        mSaveSelectWidget->show();
        mSaveAllWidget->hide();
        mOptionsLayout->addWidget(mSaveSelectWidget);
        optionWidgetHeigth += mSaveSelectWidget->height();
   }

    mOptionsLayout->addStretch();
    mOptionsWidget->setLayout(mOptionsLayout);
    mOptionsWidget->setGeometry(0, 0, mOptionsW - mMargin, optionWidgetHeigth);
}

#pragma mark Utilities



bool ResultsView::xScaleRepresentsTime()
{
    return (mCurrentTypeGraph == GraphViewResults::ePostDistrib) && ( mMainVariable == GraphViewResults::eBeginEnd||
                                     mMainVariable == GraphViewResults::eThetaEvent ||
                                     mMainVariable == GraphViewResults::eTempo ||
                                     mMainVariable == GraphViewResults::eActivity ||
                                     mMainVariable == GraphViewResults::eG ||
                                     mMainVariable == GraphViewResults::eGP ||
                                     mMainVariable == GraphViewResults::eGS );
}

double ResultsView::sliderToZoom(const int coef)
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
    const double prop = mZoomSpin->value() / 100.;
    mGraphHeight = min + prop * (origin - min);
    updateGraphsLayout();
}

void ResultsView::updateZoomT()
{
    // Pick the value from th spin or the slider
    const double zoom = mTimeSpin->value();

    mResultZoomT = 1./zoom;

    const double tCenter = (mResultCurrentMaxT + mResultCurrentMinT)/2.;
    const double span = (mResultMaxT - mResultMinT)* (1./ zoom);

    double curMin = tCenter - span/2.;
    double curMax = tCenter + span/2.;

    if (curMin < mRuler->mMin) {
        curMin = mRuler->mMin;
        curMax = curMin + span;

    } else if (curMax > mRuler->mMax) {
        curMax = mRuler->mMax;
        curMin = curMax - span;
    }

    mResultCurrentMinT = curMin; //std::max(curMin, mResultMinT);
    mResultCurrentMaxT = curMax; //std::min(curMax, mResultMaxT);;

    mRuler->setCurrent(mResultCurrentMinT, mResultCurrentMaxT);

    setTimeRange();

    updateGraphsZoomT();
}

void ResultsView::updateGraphsZoomT()
{
    // ------------------------------------------------------
    //  Update graphs zoom and scale
    // ------------------------------------------------------
    QList<GraphViewResults*> graphs = currentGraphs(false);
    for (GraphViewResults*& graph : graphs) {
        graph->changeXScaleDivision(mMajorScale, mMinorCountScale);
        graph->zoom(mResultCurrentMinT, mResultCurrentMaxT);
    }

    // ------------------------------------------------------
    //  Store zoom and scale for this type of graph
    // ------------------------------------------------------
    QPair<GraphViewResults::variable_t, GraphViewResults::graph_t> key(mMainVariable, mCurrentTypeGraph);

    if (xScaleRepresentsTime()) {
        const double minFormatted = DateUtils::convertFromAppSettingsFormat(mResultCurrentMinT);
        const double maxFormatted = DateUtils::convertFromAppSettingsFormat(mResultCurrentMaxT);

        std::pair<double, double> resultMinMax = std::minmax(minFormatted, maxFormatted);

        mZooms[key] = QPair<double, double>(resultMinMax.first, resultMinMax.second);

    } else {
        mZooms[key] = QPair<double, double>(mResultCurrentMinT, mResultCurrentMaxT);
    }

    mScales[key] = QPair<double, int>(mMajorScale, mMinorCountScale);
}

#pragma mark Controls setters

void ResultsView::setTimeRange()
{
    QLocale locale = QLocale();

    mCurrentTMinEdit->blockSignals(true);
    mCurrentTMaxEdit->blockSignals(true);

    mCurrentTMinEdit->setText(locale.toString(mResultCurrentMinT, 'f', 0));
    mCurrentTMaxEdit->setText(locale.toString(mResultCurrentMaxT, 'f', 0));

    mCurrentTMinEdit->blockSignals(false);
    mCurrentTMaxEdit->blockSignals(false);
}

void ResultsView::setTimeSpin(const double value)
{
    mTimeSpin->blockSignals(true);
    mTimeSpin->setValue(value);
    mTimeSpin->blockSignals(false);
}

void ResultsView::setTimeSlider(const int value)
{
    mXSlider->blockSignals(true);
    mXSlider->setValue(value);
    mXSlider->blockSignals(false);
}

void ResultsView::setTimeScale()
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
    if (xScaleRepresentsTime() ||
            mMainVariable == GraphViewResults::eLambda) {
        mResultCurrentMinT = min;
        mResultCurrentMaxT = max;

    } else {
        if (mCurrentTypeGraph == GraphViewResults::ePostDistrib &&
            (mMainVariable == GraphViewResults::eSigma ||
             mMainVariable == GraphViewResults::eDuration ||
             mMainVariable == GraphViewResults::eVg  ||
             mMainVariable == GraphViewResults::eS02Vg ) ) {
                mResultCurrentMinT = std::max(min, mResultMinT);
                mResultCurrentMaxT = max;

            } else {
                mResultCurrentMinT = min; //std::max(min, mResultMinT);
                mResultCurrentMaxT = max;//std::min(max, mResultMaxT);
            }

    }
    setTimeRange();
    updateGraphsZoomT();
}

void ResultsView::applyStudyPeriod()
{
    if (xScaleRepresentsTime()) {
      mResultCurrentMinT = mModel->mSettings.getTminFormated();
      mResultCurrentMaxT = mModel->mSettings.getTmaxFormated();

    } else if ( mMainVariable == GraphViewResults::eSigma ||
                mMainVariable == GraphViewResults::eDuration ||
                mMainVariable == GraphViewResults::eVg  ||
                mMainVariable == GraphViewResults::eS02Vg ) {
        mResultCurrentMinT = 0.;
        mResultCurrentMaxT = mResultMaxT;

    } else if ( mMainVariable == GraphViewResults::eLambda) {
        mResultCurrentMinT = mResultMinT;
        mResultCurrentMaxT = mResultMaxT;

    } else if (mCurrentTypeGraph == GraphViewResults::eCorrel) {
        mResultCurrentMinT = 0;
        mResultCurrentMaxT = 40;

    } else if ( mCurrentTypeGraph == GraphViewResults::eTrace ||
                mCurrentTypeGraph == GraphViewResults::eAccept) {
        int idSelect = 0;
        for (const auto& chRadio : mChainRadios) {
               if (chRadio->isChecked())
                      break;
               ++idSelect;
        }
        const ChainSpecs& chain = mModel->mChains.at(idSelect);
        const int adaptSize = chain.mBatchIndex * chain.mIterPerBatch;
        const int runSize = chain.mRealyAccepted;
        // The min is always 0
        mResultCurrentMinT = 0.;
        mResultCurrentMaxT = 1 +  chain.mIterPerBurn + adaptSize + runSize;

    }
        
    if (xScaleRepresentsTime()) {
        //auto span = (mModel->mSettings.getTmaxFormated() - mModel->mSettings.getTminFormated())/2. * mTimeSpin->minimum();
        //mResultMinT = mModel->mSettings.getTminFormated() - span ;
        //mResultMaxT = mModel->mSettings.getTmaxFormated() + span ;

        mResultZoomT = (mResultMaxT - mResultMinT)/(mResultCurrentMaxT - mResultCurrentMinT);

    } else
        mResultZoomT = (mResultMaxT - mResultMinT)/(mResultCurrentMaxT - mResultCurrentMinT);

    Scale Xscale;
    Xscale.findOptimalMark(mResultCurrentMinT, mResultCurrentMaxT, 10);

    mMajorScale = Xscale.mark;
    mMinorCountScale = Xscale.tip;

    mRuler->setScaleDivision(Xscale);
    mRuler->setCurrent(mResultCurrentMinT, mResultCurrentMaxT);

    updateGraphsZoomT();

    setTimeRange();
    setTimeSlider(zoomToSlider(mResultZoomT));
    setTimeSpin(mResultZoomT);
    setTimeScale();

}

void ResultsView::applyTimeRange()
{
    // --------------------------------------------------
    //  Find new current min & max (check range validity !)
    //  Update mResultZoomT
    // --------------------------------------------------
    QString minStr = mCurrentTMinEdit->text();
    bool minIsNumber = true;
    double min = locale().toDouble(minStr, &minIsNumber);

    QString maxStr = mCurrentTMaxEdit->text();
    bool maxIsNumber = true;
    double max = locale().toDouble(maxStr, &maxIsNumber);

    if (minIsNumber && maxIsNumber && ((min != mResultCurrentMinT) || (max != mResultCurrentMaxT))) {
        mResultCurrentMinT = std::max(min, mRuler->mMin);
        mResultCurrentMaxT = std::min(max, mRuler->mMax);

        mResultZoomT = (mResultMaxT - mResultMinT)/ (mResultCurrentMaxT - mResultCurrentMinT);

        mRuler->setCurrent(mResultCurrentMinT, mResultCurrentMaxT);

        updateGraphsZoomT();

        setTimeRange();
        setTimeSlider(zoomToSlider(mResultZoomT));
        setTimeSpin(mResultZoomT);
    }
}

void ResultsView::applyTimeSlider(int value)
{
    setTimeSpin(sliderToZoom(value));
    updateZoomT();
}

void ResultsView::applyTimeSpin(double value)
{
    setTimeSlider(zoomToSlider(value));
    updateZoomT();
}

# pragma mark curve Zoom
void ResultsView::applyXRange()
{
    bool minIsNumber = true;
    const double minX = locale().toDouble(mCurrentXMinEdit->text(), &minIsNumber);

    bool maxIsNumber = true;
    const double maxX = locale().toDouble(mCurrentXMaxEdit->text(), &maxIsNumber);
    if (minIsNumber && maxIsNumber && minX< maxX) {
        mResultCurrentMinX = minX;
        mResultCurrentMaxX = maxX;
        setXRange();

        updateCurvesToShow();
    }

}

void ResultsView::applyYRange()
{
    bool minIsNumber = true;
    const double minY = locale().toDouble(mCurrentYMinEdit->text(), &minIsNumber);

    bool maxIsNumber = true;
    const double maxY = locale().toDouble(mCurrentYMaxEdit->text(), &maxIsNumber);
    if (minIsNumber && maxIsNumber && minY< maxY) {
        mResultCurrentMinY = minY;
        mResultCurrentMaxY = maxY;
        setYRange();

        updateCurvesToShow();
    }

}

void ResultsView::applyZRange()
{
    bool minIsNumber = true;
    const double minZ = locale().toDouble(mCurrentZMinEdit->text(), &minIsNumber);

    bool maxIsNumber = true;
    const double maxZ = locale().toDouble(mCurrentZMaxEdit->text(), &maxIsNumber);
    if (minIsNumber && maxIsNumber && minZ< maxZ) {
        mResultCurrentMinZ = minZ;
        mResultCurrentMaxZ = maxZ;
        setZRange();

        updateCurvesToShow();
    }

}


void ResultsView::findOptimalX()
{
    const ModelCurve* modelCurve = static_cast<const ModelCurve*> (mModel);
    const std::vector<double>* vec = nullptr;

    Scale XScale;
    if (mCurveGRadio->isChecked()) {
        vec = &modelCurve->mPosteriorMeanG.gx.vecG;
        const std::vector<double>* vecVar = &modelCurve->mPosteriorMeanG.gx.vecVarG;

        double minY = +INFINITY;
        double maxY = -INFINITY;
        minY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), minY, [](double x, Event* e) {return std::min(e->mXIncDepth, x);});
        maxY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), maxY, [](double x, Event* e) {return std::max(e->mXIncDepth, x);});
        int i = 0;
        for (auto g : *vec) {
            const auto e = 1.96*sqrt(vecVar->at(i));
            minY = std::min(minY, g - e);
            maxY = std::max(maxY, g + e);
            i++;
        }

        XScale.findOptimal(minY, maxY, 7);

    } else {
        if (mCurveGPRadio->isChecked())
            vec = &modelCurve->mPosteriorMeanG.gx.vecGP;
        else
            vec = &modelCurve->mPosteriorMeanG.gx.vecGS;

        const auto minMax = std::minmax_element(vec->begin(), vec->end());

        XScale.findOptimal(*minMax.first, *minMax.second, 7);
    }


    mResultCurrentMinX = XScale.min;
    mResultCurrentMaxX = XScale.max;

    setXRange();

    updateCurvesToShow();

}

void ResultsView::findOptimalY()
{
    const ModelCurve* modelCurve = static_cast<const ModelCurve*> (mModel);
    const std::vector<double>* vec = nullptr;

    Scale XScale;
    if (mCurveGRadio->isChecked()) {
        vec = &modelCurve->mPosteriorMeanG.gy.vecG;
        const std::vector<double>* vecVar = &modelCurve->mPosteriorMeanG.gy.vecVarG;

        double minY = +INFINITY;
        double maxY = -INFINITY;
        minY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), minY, [](double x, Event* e) {return std::min(e->mYDec, x);});
        maxY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), maxY, [](double x, Event* e) {return std::max(e->mYDec, x);});
        int i = 0;
        for (auto g : *vec) {
            const auto e = 1.96*sqrt(vecVar->at(i));
            minY = std::min(minY, g - e);
            maxY = std::max(maxY, g + e);
            i++;
        }

        XScale.findOptimal(minY, maxY, 7);

    } else {
        if (mCurveGPRadio->isChecked())
            vec = &modelCurve->mPosteriorMeanG.gy.vecGP;
        else
            vec = &modelCurve->mPosteriorMeanG.gy.vecGS;

        const auto minMax = std::minmax_element(vec->begin(), vec->end());

        XScale.findOptimal(*minMax.first, *minMax.second, 7);
    }


    mResultCurrentMinY = XScale.min;
    mResultCurrentMaxY = XScale.max;
    setYRange();

    updateCurvesToShow();
}

void ResultsView::findOptimalZ()
{
    const ModelCurve* modelCurve = static_cast<const ModelCurve*> (mModel);

    const std::vector<double>* vec = nullptr;
    Scale XScale;
    if (mCurveGRadio->isChecked()) {
        vec = &modelCurve->mPosteriorMeanG.gz.vecG;
        const std::vector<double>* vecVar = &modelCurve->mPosteriorMeanG.gz.vecVarG;

        double minY = +INFINITY;
        double maxY = -INFINITY;
        minY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), minY, [](double x, Event* e) {return std::min(e->mZField, x);});
        maxY = std::accumulate(mModel->mEvents.begin(), mModel->mEvents.end(), maxY, [](double x, Event* e) {return std::max(e->mZField, x);});
        int i = 0;
        for (auto g : *vec) {
            const auto e = 1.96*sqrt(vecVar->at(i));
            minY = std::min(minY, g - e);
            maxY = std::max(maxY, g + e);
            i++;
        }

        XScale.findOptimal(minY, maxY, 7);

    } else {
        if (mCurveGPRadio->isChecked())
            vec = &modelCurve->mPosteriorMeanG.gz.vecGP;
        else
            vec = &modelCurve->mPosteriorMeanG.gz.vecGS;

        const auto minMax = std::minmax_element(vec->begin(), vec->end());

        XScale.findOptimal(*minMax.first, *minMax.second, 7);
    }

    mResultCurrentMinZ = XScale.min;
    mResultCurrentMaxZ = XScale.max;

    setZRange();
    updateCurvesToShow();
}

void ResultsView::setXRange()
{
    QLocale locale = QLocale();

    mCurrentXMinEdit->blockSignals(true);
    mCurrentXMaxEdit->blockSignals(true);

    mCurrentXMinEdit->setText(locale.toString(mResultCurrentMinX));
    mCurrentXMaxEdit->setText(locale.toString(mResultCurrentMaxX));

    mCurrentXMinEdit->blockSignals(false);
    mCurrentXMaxEdit->blockSignals(false);

    QPair<GraphViewResults::variable_t, GraphViewResults::graph_t> key(mMainVariable, mCurrentTypeGraph);
    mZoomsX[key] = QPair<double, double>(mResultCurrentMinX, mResultCurrentMaxX);
}

void ResultsView::setYRange()
{
    QLocale locale = QLocale();

    mCurrentYMinEdit->blockSignals(true);
    mCurrentYMaxEdit->blockSignals(true);

    mCurrentYMinEdit->setText(locale.toString(mResultCurrentMinY));
    mCurrentYMaxEdit->setText(locale.toString(mResultCurrentMaxY));

    mCurrentYMinEdit->blockSignals(false);
    mCurrentYMaxEdit->blockSignals(false);

    QPair<GraphViewResults::variable_t, GraphViewResults::graph_t> key(mMainVariable, mCurrentTypeGraph);
    mZoomsY[key] = QPair<double, double>(mResultCurrentMinY, mResultCurrentMaxY);
}

void ResultsView::setZRange()
{
    QLocale locale = QLocale();

    mCurrentZMinEdit->blockSignals(true);
    mCurrentZMaxEdit->blockSignals(true);

    mCurrentZMinEdit->setText(locale.toString(mResultCurrentMinZ));
    mCurrentZMaxEdit->setText(locale.toString(mResultCurrentMaxZ));

    mCurrentZMinEdit->blockSignals(false);
    mCurrentZMaxEdit->blockSignals(false);

    QPair<GraphViewResults::variable_t, GraphViewResults::graph_t> key(mMainVariable, mCurrentTypeGraph);
    mZoomsZ[key] = QPair<double, double>(mResultCurrentMinZ, mResultCurrentMaxZ);
}

void ResultsView::applyZoomScale()
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

    QPair<GraphViewResults::variable_t, GraphViewResults::graph_t> key(mMainVariable, mCurrentTypeGraph);
    mScales[key] = QPair<double, int>(mMajorScale, mMinorCountScale);
}

void ResultsView::applyZoomSlider(int value)
{
    mZoomSpin->blockSignals(true);
    mZoomSpin->setValue(value);
    mZoomSpin->blockSignals(false);

    updateGraphsHeight();
}

void ResultsView::applyZoomSpin(int value)
{
    mZoomSlider->blockSignals(true);
    mZoomSlider->setValue(value);
    mZoomSlider->blockSignals(false);

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

void ResultsView::applyHActivity()
{
    const double h = locale().toDouble(mHActivityEdit->text());
    const double rangePercent = locale().toDouble(mRangeThresholdEdit->text());
    mModel->setHActivity(h, rangePercent);
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

void ResultsView::showStats(bool show)
{
    mPhasesStatCheck->setChecked(show);
    mStatCheck->setChecked(show);
    mCurveStatCheck->setChecked(show);
    
    QList<GraphViewResults*> graphs = allGraphs();
    for (GraphViewResults*& graph : graphs) {
        graph->showNumericalResults(show);
    }
    updateLayout();
}

#pragma mark Graph selection and export
// Obsolete
void ResultsView::togglePageSave()
{

        // Exchange with the widget corresponding to the requested tab
        if (mPageSaveTab->currentName() == tr("Page") ) { // Tab = Page
            // -------------------------------------------------------------------------------------
            //  - Update the total number of graphs for all pages
            //  - Check if the current page is still lower than the number of pages
            //  - Update the pagination display
            //  => All this must be done BEFORE calling createGraphs, which uses theses params to build the graphs
            // -------------------------------------------------------------------------------------
            updateTotalGraphs();

            const int numPages = ceil((double)mMaximunNumberOfVisibleGraph / (double)mGraphsPerPage);
            if (mCurrentPage >= numPages) {
                mCurrentPage = 0;
            }

            mPageEdit->setText(locale().toString(mCurrentPage + 1) + "/" + locale().toString(numPages));

            mPageWidget->setVisible(true);
            mSaveSelectWidget->hide();
            mSaveAllWidget->hide();

         //   if (widFrom != mPageWidget)
             //   mOptionsLayout->replaceWidget(widFrom, mPageWidget);

        } else if (mPageSaveTab->currentName() == tr("Save") ) { // {  Tab = Saving
            mPageWidget->hide();

            const bool hasSelection = (currentGraphs(true).size() > 0);

            if (hasSelection) {// && widFrom != mSaveSelectWidget) {
               // mOptionsLayout->replaceWidget(widFrom, mSaveSelectWidget);
                mSaveAllWidget->hide();
                mSaveSelectWidget->show();

            } else { //if (!hasSelection && widFrom != mSaveAllWidget) {
              //  mOptionsLayout->replaceWidget(widFrom, mSaveAllWidget);
                mSaveSelectWidget->hide();
                mSaveAllWidget->show();

            }

        }
  //  }

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
    for (const auto& graph : graphs) {
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
        QObject::tr("Image (*.png);;Photo (*.jpg);; Windows Bitmap (*.bmp);;Scalable Vector Graphics (*.svg)"));

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

            // --------------   Saving Curve Map
            if (mModel->mProject->isCurve()) {
                // first Map
                const auto list_names = mModel->getCurvesName();

                file.setFileName(dirPath + "/"+list_names.at(0) + "_Curve_Map.csv");

                if (file.open(QFile::WriteOnly | QFile::Truncate)) {
                    modelCurve()->saveMapToFile(&file, csvSep, modelCurve()->mPosteriorMeanG.gx.mapG);

                }

                if (mModel->displayY()) {
                    file.setFileName(dirPath + "/"+list_names.at(1) + "_Curve_Map.csv");

                    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
                        modelCurve()->saveMapToFile(&file, csvSep, modelCurve()->mPosteriorMeanG.gy.mapG);
                        file.close();
                    }

                    if (mModel->displayZ()) {
                        file.setFileName(dirPath + "/"+list_names.at(2) + "_Curve_Map.csv");

                        if (file.open(QFile::WriteOnly | QFile::Truncate)) {
                            modelCurve()->saveMapToFile(&file, csvSep, modelCurve()->mPosteriorMeanG.gz.mapG);
                            file.close();
                        }
                    }

                }

                // --------------   Saving Curve Ref
                int i = 0;
                for (auto&& graph : mByCurveGraphs) {
                    graph->getGraph()->exportReferenceCurves ("", QLocale::English, ",",  mModel->mSettings.mStep, dirPath + "/"+list_names.at(i) + "_ref.csv" );
                    i++;
                }


            }
        }
    }
}

void ResultsView::exportFullImage()
{
    bool printAxis = (mGraphHeight < GraphViewResults::mHeightForVisibleAxis);

    QWidget* curWid (nullptr);

    if (mGraphListTab->currentName() == tr("Events")) {
        curWid = mEventsScrollArea->widget();
        curWid->setFont(mByEventsGraphs.at(0)->font());

    } else if (mGraphListTab->currentName() == tr("Phases")) {
        curWid = mPhasesScrollArea->widget();
        curWid->setFont(mByPhasesGraphs.at(0)->font());

    } else if (mGraphListTab->currentName() == tr("Curves")) {
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
            axisWidget->updateValues(int (curWid->width()*2./3. - axisWidget->mMarginLeft - axisWidget->mMarginRight), 50, mResultCurrentMinT, mResultCurrentMaxT);

        } else {
            axisWidget->setGeometry(0, curWid->height() - axeHeight, curWid->width(), axeHeight);
            axisWidget->updateValues(int (curWid->width() - axisWidget->mMarginLeft - axisWidget->mMarginRight), 50, mResultCurrentMinT, mResultCurrentMaxT);
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
        if (mCurrentTypeGraph == GraphViewResults::ePostDistrib && ( mCurrentVariableList.contains(GraphViewResults::eBeginEnd) ||
                                                                     mCurrentVariableList.contains(GraphViewResults::eThetaEvent) ||
                                                                     mCurrentVariableList.contains(GraphViewResults::eActivity)||
                                                                     mCurrentVariableList.contains(GraphViewResults::eG) ||
                                                                     mCurrentVariableList.contains(GraphViewResults::eGP) ||
                                                                     mCurrentVariableList.contains(GraphViewResults::eGS)) )
            legend = DateUtils::getAppSettingsFormatStr();

        else if (mCurrentTypeGraph == GraphViewResults::eTrace || mCurrentTypeGraph == GraphViewResults::eAccept)
            legend = "Iterations";

        else if (mCurrentTypeGraph == GraphViewResults::ePostDistrib && mCurrentVariableList.contains(GraphViewResults::eDuration))
            legend = "Years";


        axisLegend = new QLabel(legend, curWid);

        axisLegend->setFont(font());
        QFontMetrics fm(font());
        if (mStatCheck->isChecked())
            axisLegend->setGeometry(fm.horizontalAdvance(legend), curWid->height() - axeHeight - legendHeight, int (curWid->width()*2./3. - 10), legendHeight);
        else
            axisLegend->setGeometry(int (curWid->width() - fm.horizontalAdvance(legend) - mMarginRight), curWid->height() - axeHeight - legendHeight, fm.horizontalAdvance(legend) , legendHeight);

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

#pragma mark Curve

bool ResultsView::isCurve() const
{
    if (mModel)
        return mModel->mProject->isCurve();
    else
        return false;
}

/* dynamic_cast can only be used with pointers and references.
 *  On failure to cast, a null pointer is returned.
 */
ModelCurve* ResultsView::modelCurve() const
{
    return dynamic_cast<ModelCurve*>(mModel);
}

GraphViewResults::variable_t ResultsView::getMainVariable() const
{

    if (mCurrentVariableList.contains(GraphViewResults::eThetaEvent))
        return GraphViewResults::eThetaEvent;
    else if (mCurrentVariableList.contains(GraphViewResults::eSigma))
        return GraphViewResults::eSigma;
    else if (mCurrentVariableList.contains(GraphViewResults::eVg))
        return GraphViewResults::eVg;
    else if (mCurrentVariableList.contains(GraphViewResults::eSigma))
        return GraphViewResults::eSigma;

    else if (mCurrentVariableList.contains(GraphViewResults::eBeginEnd))
        return GraphViewResults::eBeginEnd;
    else if (mCurrentVariableList.contains(GraphViewResults::eTempo))
        return GraphViewResults::eTempo;
    else if (mCurrentVariableList.contains(GraphViewResults::eActivity))
        return GraphViewResults::eActivity;
    else if (mCurrentVariableList.contains(GraphViewResults::eDuration))
        return GraphViewResults::eDuration;

    else if (mCurrentVariableList.contains(GraphViewResults::eG))
        return GraphViewResults::eG;
    else if (mCurrentVariableList.contains(GraphViewResults::eGP))
        return GraphViewResults::eGP;
    else if (mCurrentVariableList.contains(GraphViewResults::eGS))
        return GraphViewResults::eGS;
    else if (mCurrentVariableList.contains(GraphViewResults::eLambda))
        return GraphViewResults::eLambda;
    else if (mCurrentVariableList.contains(GraphViewResults::eS02Vg))
        return GraphViewResults::eS02Vg;
    else
        return GraphViewResults::eThetaEvent;
}
