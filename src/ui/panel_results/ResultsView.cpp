/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2025

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

#ifdef KOMLAN
 #include "GraphViewS02Vg.h"
#endif

#include "ProjectView.h"

#include "StdUtilities.h"
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
#include "DoubleValidator.h"

#include "MainWindow.h"
#include "Project.h"

#include "QtUtilities.h"
#include "ModelUtilities.h"
#include "AppSettings.h"

#include <QFontDialog>
#include <QScrollBar>
#include <QSpinBox>
#include <QMouseEvent>
#include <QClipboard>
#include <QFileDialog>
#include <QDir>
#include <QSvgGenerator>

constexpr int h_Title = 20;
constexpr int h_Edit = 20;
constexpr int h_Label = 20;
constexpr int h_Check = 15;
constexpr int h_Radio = 15;
constexpr int h_Tab = 40;
constexpr int h_Button = 25;
constexpr int h_Slider = 25;
constexpr int h_Combo = 20;

ResultsView::ResultsView(QWidget* parent, Qt::WindowFlags flags):
    QWidget(parent, flags),
    mMargin(5),
    mOptionsW(250),
    mMarginLeft(40),
    mMarginRight(40),

    mCurrentTypeGraph(GraphViewResults::ePostDistrib),
    mCurrentVariableList(GraphViewResults::eThetaEvent),
    mMainVariable(GraphViewResults::eThetaEvent),
    mHasPhases(false),
    mHpdThreshold(95.0),

    mResultZoomT(1.0),
    mResultMinT(0.0),
    mResultMaxT(0.0),
    mResultCurrentMinT(0.0),

    mResultCurrentMaxT(0.0),
    mMajorScale(100),

    mMinorCountScale(4),
    mCurrentPage(0),
    mGraphsPerPage(APP_SETTINGS_DEFAULT_SHEET),
    mMaximunNumberOfVisibleGraph(0)
{
    setMouseTracking(true);

    QDoubleValidator* RplusValidator = new QDoubleValidator(this);
    RplusValidator->setBottom(0.000001);

    // -----------------------------------------------------------------
    //  Left part : Tabs, Ruler, Stack
    // -----------------------------------------------------------------
    mGraphTypeTabs = new Tabs(this);
    mGraphTypeTabs->addTab(tr("Posterior Distrib."));
    mGraphTypeTabs->addTab(tr("History Plot"));
    mGraphTypeTabs->addTab(tr("Acceptance Rate"));
    mGraphTypeTabs->addTab(tr("Autocorrelation"));
    mGraphTypeTabs->setTab(0, false);
    mGraphTypeTabs->setFixedHeight(h_Tab);

    mRuler = new Ruler(this);
    mRuler->setMarginLeft(mMarginLeft);
    mRuler->setMarginRight(mMarginRight);

    mMarker = new Marker(this);

    mEventsWidget = new QWidget(this);
    mEventsWidget->setMouseTracking(true);

    mEventsScrollArea = new QScrollArea(this);
    mEventsScrollArea->QScrollArea::setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mEventsScrollArea->horizontalScrollBar()->setEnabled(false);
    mEventsScrollArea->setMouseTracking(true);
    mEventsScrollArea->setWidget(mEventsWidget);

    mPhasesWidget = new QWidget(this);
    mPhasesWidget->setMouseTracking(true);

    mPhasesScrollArea = new QScrollArea(this);
    mPhasesScrollArea->QScrollArea::setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mPhasesScrollArea->horizontalScrollBar()->setEnabled(false);
    mPhasesScrollArea->setMouseTracking(true);
    mPhasesScrollArea->setWidget(mPhasesWidget);

    mCurvesWidget = new QWidget(this);
    mCurvesWidget->setMouseTracking(true);

    mCurvesScrollArea = new QScrollArea(this);
    mCurvesScrollArea->QScrollArea::setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mCurvesScrollArea->horizontalScrollBar()->setEnabled(false);
    mCurvesScrollArea->setMouseTracking(true);
    mCurvesScrollArea->setWidget(mCurvesWidget);

    connect(mGraphTypeTabs, static_cast<void (Tabs::*)(const qsizetype&)>(&Tabs::tabClicked), this, &ResultsView::applyGraphTypeTab);
    connect(mRuler, &Ruler::positionChanged, this, &ResultsView::applyRuler);

   // connect(this, &ResultsView::wheelMove, mRuler, &Ruler::wheelScene);// Allows horizontal scrolling on the results window, without having to use the ruler

    // -----------------------------------------------------------------
    //  Right Part
    // -----------------------------------------------------------------
    mOptionsScroll = new QScrollArea(this);
    mOptionsScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mOptionsWidget = new QWidget(this);
    mOptionsScroll->setWidget(mOptionsWidget);

    // -----------------------------------------------------------------
    //  Results Group (if graph list tab = events or phases)
    // -----------------------------------------------------------------
    mEventsGroup = new QWidget(this);

    mEventsDatesUnfoldCheck = new CheckBox(tr("Unfold Data"));
    mEventsDatesUnfoldCheck->setFixedHeight(h_Check);
    mEventsDatesUnfoldCheck->setToolTip(tr("Display Events' data"));

    mEventThetaRadio = new RadioButton(tr("Event Date"));
    mEventThetaRadio->setFixedHeight(h_Radio);
    mEventThetaRadio->setChecked(true);

    mDataSigmaRadio = new RadioButton(tr("Std ti"));
    mDataSigmaRadio->setFixedHeight(h_Radio);
#ifdef S02_BAYESIAN
    mS02Radio = new RadioButton(tr("Shrinkage Param."));
    mS02Radio->setFixedHeight(h_Radio);

#ifdef KOMLAN
    mS02VgRadio = new RadioButton(tr("S02 Vg Param."));
    mS02VgRadio->setFixedHeight(h_Radio);
#endif

#endif


    mEventVGRadio = new RadioButton(tr("Std gi"));
    mEventVGRadio->setFixedHeight(h_Radio);

    mDataCalibCheck = new CheckBox(tr("Individual Calib. Dates"));
    mDataCalibCheck->setFixedHeight(h_Check);
    mDataCalibCheck->setChecked(true);

    mWiggleCheck = new CheckBox(tr("Wiggle shifted"));
    mWiggleCheck->setFixedHeight(h_Check);

    mEventsStatCheck = new CheckBox(tr("Show Stat."));
    mEventsStatCheck->setFixedHeight(h_Check);
    mEventsStatCheck->setToolTip(tr("Display numerical results computed on posterior densities below all graphs."));

    QVBoxLayout* resultsGroupLayout = new QVBoxLayout();
    resultsGroupLayout->setContentsMargins(10, 10, 10, 10);
    resultsGroupLayout->setSpacing(15);
    resultsGroupLayout->addWidget(mEventThetaRadio);
    resultsGroupLayout->addWidget(mDataSigmaRadio);

#ifdef S02_BAYESIAN
    resultsGroupLayout->addWidget(mS02Radio);
 #ifdef KOMLAN
    resultsGroupLayout->addWidget(mS02VgRadio);
 #endif
#endif

    resultsGroupLayout->addWidget(mEventVGRadio);

    resultsGroupLayout->addWidget(mEventsDatesUnfoldCheck);
    resultsGroupLayout->addWidget(mDataCalibCheck);
    resultsGroupLayout->addWidget(mWiggleCheck);
    resultsGroupLayout->addWidget(mEventsStatCheck);

    //mEventsGroup->resize(8 * h, mOptionsW);
    mEventsGroup->setLayout(resultsGroupLayout);


    // -----------------------------------------------------------------
    //  Tempo Group (if graph list tab = duration)
    // -----------------------------------------------------------------
    mPhasesGroup = new QWidget();
    mBeginEndRadio = new RadioButton(tr("Begin-End"), mPhasesGroup);
    mBeginEndRadio->setFixedHeight(h_Radio);
    mBeginEndRadio->setChecked(true);

    mPhasesEventsUnfoldCheck = new CheckBox(tr("Unfold Events"), mPhasesGroup);
    mPhasesEventsUnfoldCheck->setFixedHeight(h_Check);
    mPhasesEventsUnfoldCheck->setToolTip(tr("Display Phases' Events"));

    mPhasesDatesUnfoldCheck = new CheckBox(tr("Unfold Data"), mPhasesGroup);
    mPhasesDatesUnfoldCheck->setFixedHeight(h_Check);
    mPhasesDatesUnfoldCheck->setToolTip(tr("Display Events' Data"));

    mTempoRadio = new RadioButton(tr("Tempo"), mPhasesGroup);
    mTempoRadio->setFixedHeight(h_Radio);

    mActivityRadio = new RadioButton(tr("Activity"), mPhasesGroup);
    mActivityRadio->setFixedHeight(h_Radio);

    mActivityUnifCheck = new CheckBox(tr("Unif Theo"), mPhasesGroup);
    mActivityUnifCheck->setFixedHeight(h_Check);

    mErrCheck = new CheckBox(tr("Error (Clopper-Pearson)"), mPhasesGroup);
    mErrCheck->setFixedHeight(h_Check);

    mDurationRadio = new RadioButton(tr("Duration"), mPhasesGroup);
    mDurationRadio->setFixedHeight(h_Radio);
    mDurationRadio->setChecked(false);

    mPhasesStatCheck = new CheckBox(tr("Show Stat."));
    mPhasesStatCheck->setFixedHeight(h_Check);
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

    auto threshold_str = stringForLocal(mHpdThreshold);
    mCurveGRadio = new RadioButton(tr("Curve (at %1% Level)").arg(threshold_str), mCurvesGroup);
    mCurveGRadio->setFixedHeight(h_Radio);
    mCurveGRadio->setChecked(true);

    mCurveHpdCheck = new CheckBox(tr("HPD Envelope"), mCurvesGroup);
    mCurveHpdCheck->setFixedHeight(h_Check);
    mCurveHpdCheck->setChecked(true);

    mCurveMapCheck = new CheckBox(tr("Density Plot"), mCurvesGroup);
    mCurveMapCheck->setFixedHeight(h_Check);
    mCurveMapCheck->setChecked(true);

    mCurveDataPointsCheck = new CheckBox(tr("Reference Points (HPD Interval)"), mCurvesGroup);
    mCurveDataPointsCheck->setFixedHeight(h_Check);
    mCurveDataPointsCheck->setChecked(true);

    mCurveErrorCheck = new CheckBox(tr("Gauss Envelope"), mCurvesGroup);
    mCurveErrorCheck->setFixedHeight(h_Check);
    mCurveErrorCheck->setChecked(false);
    
    mCurveEventsPointsCheck = new CheckBox(tr("Event Dates (HPD Interval)"), mCurvesGroup);
    mCurveEventsPointsCheck->setFixedHeight(h_Check);
    mCurveEventsPointsCheck->setChecked(false);

    // G Prime Button
    mCurveGPRadio = new RadioButton(tr("Speed of Change (Derivative) (at %1% Level)").arg(threshold_str), mCurvesGroup);
    mCurveGPRadio->setFixedHeight(h_Radio);

    mCurveGPGaussCheck = new CheckBox(tr("Gauss Envelope"), mCurvesGroup);
    mCurveGPGaussCheck->setFixedHeight(h_Check);
    mCurveGPGaussCheck->setChecked(true);

    mCurveGPHpdCheck = new CheckBox(tr("HPD Envelope"), mCurvesGroup);
    mCurveGPHpdCheck->setFixedHeight(h_Check);
    mCurveGPHpdCheck->setChecked(true);

    mCurveGPMapCheck = new CheckBox(tr("Density Plot"), mCurvesGroup);
    mCurveGPMapCheck->setFixedHeight(h_Check);
    mCurveGPMapCheck->setChecked(true);

    // G second Derivative Button
    mCurveGSRadio = new RadioButton(tr("Acceleration"), mCurvesGroup);
    mCurveGSRadio->setFixedHeight(h_Radio);
    
    mLambdaRadio = new RadioButton(tr("Smoothing"), mCurvesGroup);
    mLambdaRadio->setFixedHeight(h_Radio);

    mCurveStatCheck = new CheckBox(tr("Show Stat."));
    mCurveStatCheck->setFixedHeight(h_Check);
    mCurveStatCheck->setToolTip(tr("Display numerical results computed on posterior densities below all graphs."));

    QVBoxLayout* curveGroupLayout = new QVBoxLayout();

    QVBoxLayout* curveOptionGroupLayout = new QVBoxLayout();
    curveOptionGroupLayout->setContentsMargins(15, 0, 0, 0);
    curveOptionGroupLayout->addWidget(mCurveEventsPointsCheck, Qt::AlignLeft);
    curveOptionGroupLayout->addWidget(mCurveMapCheck, Qt::AlignLeft);
    curveOptionGroupLayout->addWidget(mCurveHpdCheck, Qt::AlignLeft);
    curveOptionGroupLayout->addWidget(mCurveDataPointsCheck, Qt::AlignLeft);
    curveOptionGroupLayout->addWidget(mCurveErrorCheck, Qt::AlignLeft);

    curveGroupLayout->setContentsMargins(10, 10, 10, 10);
    curveGroupLayout->addWidget(mCurveGRadio);
    curveGroupLayout->setSpacing(7);
    curveGroupLayout->addLayout(curveOptionGroupLayout);
    curveGroupLayout->addWidget(mCurveGPRadio);
    curveGroupLayout->addWidget(mCurveGSRadio);
    curveGroupLayout->addWidget(mLambdaRadio);
    curveGroupLayout->addWidget(mCurveStatCheck);

    mCurvesGroup->setLayout(curveGroupLayout);

    // -----------------------------------------------------------------
    //  Connections
    // -----------------------------------------------------------------
    connect(mEventThetaRadio, &RadioButton::clicked, this, &ResultsView::applyCurrentVariable);
#ifdef S02_BAYESIAN
    connect(mS02Radio, &RadioButton::clicked, this, &ResultsView::applyCurrentVariable);
#ifdef KOMLAN
    connect(mS02VgRadio, &RadioButton::clicked, this, &ResultsView::applyCurrentVariable);
#endif

#endif
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

    connect(mEventsStatCheck, &CheckBox::clicked, this, &ResultsView::showStats);

    connect(mErrCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mActivityUnifCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);

    connect(mPhasesStatCheck, &CheckBox::clicked, this, &ResultsView::showStats);
    connect(mCurveStatCheck, &CheckBox::clicked, this, &ResultsView::showStats);
    
    connect(mCurveGRadio, &CheckBox::clicked, this, &ResultsView::applyCurrentVariable);
    connect(mCurveGPRadio, &CheckBox::clicked, this, &ResultsView::applyCurrentVariable);
    connect(mCurveGSRadio, &CheckBox::clicked, this, &ResultsView::applyCurrentVariable);
    connect(mLambdaRadio, &CheckBox::clicked, this, &ResultsView::applyCurrentVariable);

    connect(mCurveErrorCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mCurveHpdCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mCurveMapCheck, &CheckBox::clicked, this,  &ResultsView::updateCurvesToShow);
    connect(mCurveEventsPointsCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mCurveDataPointsCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);

    connect(mCurveGPGaussCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mCurveGPHpdCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mCurveGPMapCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);

    // -----------------------------------------------------------------
    //  Graph List tab (has to be created after mResultsGroup and mTempoGroup)
    // -----------------------------------------------------------------
    mGraphListTab = new Tabs(this);
    mGraphListTab->setFixedHeight(h_Tab);
    mGraphListTab->addTab( tr("Events"));
    mGraphListTab->addTab(tr("Phases"));
    mGraphListTab->addTab(tr("Curves"));

    connect(mGraphListTab, static_cast<void (Tabs::*)(const qsizetype&)>(&Tabs::tabClicked), this, &ResultsView::applyGraphListTab);
    connect(mGraphListTab, static_cast<void (Tabs::*)(const qsizetype&)>(&Tabs::tabClicked), this, &ResultsView::updateOptionsWidget);

    // -----------------------------------------------------------------
    //  Tabs : Display / Distrib. Options
    // -----------------------------------------------------------------
    mDisplayDistribTab = new Tabs(this);
    mDisplayDistribTab->setFixedHeight(h_Tab);

    mDisplayDistribTab->addTab(tr("Display"));
    mDisplayDistribTab->addTab(tr("Distrib. Options"));

    // Necessary to reposition all elements inside the selected tab :
    connect(mDisplayDistribTab, static_cast<void (Tabs::*)(const qsizetype&)>(&Tabs::tabClicked), this, &ResultsView::updateOptionsWidget);

    // -----------------------------------------------------------------
    //  Display / Span Options
    // -----------------------------------------------------------------
    mDisplayGroup = new QWidget(this);
    mSpanGroup  = new QWidget(this);

    mSpanTitle = new Label(tr("Time Scale"), mDisplayGroup);
    mSpanTitle->setFixedHeight(h_Title);
    mSpanTitle->setIsTitle(true);

    mDisplayStudyBut = new Button(tr("Study Period Display"), mSpanGroup);
    mDisplayStudyBut->setFixedHeight(h_Button);
    mDisplayStudyBut->setToolTip(tr("Restore view with the study period span"));

    mSpanLab = new QLabel(tr("Span"), mSpanGroup);
    mSpanLab->setFixedHeight(h_Label);
    //mSpanLab->setAdjustText(false);

    mCurrentTMinEdit = new LineEdit(mSpanGroup);
    mCurrentTMinEdit->setFixedHeight(h_Edit);
    mCurrentTMinEdit->setToolTip(tr("Enter a minimal value to display the curves"));

    mCurrentTMaxEdit = new LineEdit(mSpanGroup);
    mCurrentTMaxEdit->setFixedHeight(h_Edit);
    mCurrentTMaxEdit->setToolTip(tr("Enter a maximal value to display the curves"));

    mTimeLab = new QLabel(tr("Time"), mSpanGroup);
    mTimeLab->setFixedHeight(h_Label);
    mTimeLab->setAlignment(Qt::AlignCenter);

    mTimeSlider = new QSlider(Qt::Horizontal, mSpanGroup);
    mTimeSlider->setFixedHeight(h_Slider);
    mTimeSlider->setRange(-100, 100);
    mTimeSlider->setTickInterval(1);
    mTimeSlider->setValue(0);

    mTimeEdit = new LineEdit(mSpanGroup);
    mTimeEdit->setValidator(RplusValidator);
    mTimeEdit->setFixedHeight(h_Edit);
    mTimeEdit->setText(QLocale().toString(sliderToZoom(mTimeSlider->value())));
    mTimeEdit->setToolTip(tr("Enter zoom value to magnify the curves on X span"));
    mTimeEdit->setFixedWidth(mOptionsW/3); //for windows new spin box

    mMajorScaleLab = new QLabel(tr("Major Interval"), mSpanGroup);
    mMajorScaleLab->setFixedHeight(h_Label);
    mMajorScaleLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mMajorScaleEdit = new LineEdit(mSpanGroup);
    mMajorScaleEdit->setFixedHeight(h_Edit);
    mMajorScaleEdit->setText(QString::number(mMajorScale));
    mMajorScaleEdit->setToolTip(tr("Enter an interval for the main axis division below the curves, greater than 1"));

    mMinorScaleLab = new QLabel(tr("Minor Interval Count"), mSpanGroup);
    mMinorScaleLab->setFixedHeight(h_Label);
    mMinorScaleLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mMinorScaleEdit = new LineEdit(mSpanGroup);
    mMinorScaleEdit->setFixedHeight(h_Edit);
    mMinorScaleEdit->setText(QString::number(mMinorCountScale));
    mMinorScaleEdit->setToolTip(tr("Enter a interval for the subdivision of the Major Interval for the scale under the curves, upper than 1"));

    connect(mDisplayStudyBut, static_cast<void (Button::*)(bool)>(&Button::clicked), this, &ResultsView::applyStudyPeriod);
    connect(mCurrentTMinEdit, &LineEdit::editingFinished, this, &ResultsView::applyTimeRange);
    connect(mCurrentTMaxEdit, &LineEdit::editingFinished, this, &ResultsView::applyTimeRange);
    connect(mTimeSlider, &QSlider::valueChanged, this, &ResultsView::applyTimeSlider);
    connect(mTimeEdit, &LineEdit::editingFinished, this, &ResultsView::applyTimeEdit);
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
    spanLayout2->addWidget(mTimeLab);
    spanLayout2->addWidget(mTimeSlider);
    spanLayout2->addWidget(mTimeEdit);

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

    mXOptionTitle = new Label(tr("X Scale"), mDisplayGroup);
    mXOptionTitle->setFixedHeight(h_Title);
    mXOptionTitle->setIsTitle(true);

    mXOptionBut = new Button(tr("Optimal X Display"), mXOptionGroup);
    mXOptionBut->setFixedHeight(h_Button);
    mXOptionBut->setToolTip(tr("Restore view with the study period span"));

    mXOptionLab = new QLabel(tr("X"), mXOptionGroup);
    mXOptionLab->setFixedHeight(h_Label);

    mCurrentXMinEdit = new LineEdit(mXOptionGroup);
    mCurrentXMinEdit->setFixedHeight(h_Edit);
    mCurrentXMinEdit->setToolTip(tr("Enter a minimal value to display the curves"));

    mCurrentXMaxEdit = new LineEdit(mXOptionGroup);
    mCurrentXMaxEdit->setFixedHeight(h_Edit);
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

    mYOptionTitle = new Label(tr("Y Scale"), mDisplayGroup);
    mYOptionTitle->setFixedHeight(h_Title);
    mYOptionTitle->setIsTitle(true);

    mYOptionBut = new Button(tr("Optimal Y Display"), mYOptionGroup);
    mYOptionBut->setFixedHeight(h_Button);
    mYOptionBut->setToolTip(tr("Optimize Y scale"));

    mYOptionLab = new QLabel(tr("Y"), mYOptionGroup);
    mYOptionLab->setFixedHeight(h_Label);

    mCurrentYMinEdit = new LineEdit(mYOptionGroup);
    mCurrentYMinEdit->setFixedHeight(h_Edit);
    mCurrentYMinEdit->setToolTip(tr("Enter a minimal value to display the curves"));

    mCurrentYMaxEdit = new LineEdit(mYOptionGroup);
    mCurrentYMaxEdit->setFixedHeight(h_Edit);
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

    mZOptionTitle = new Label(tr("Z Scale"), mDisplayGroup);
    mZOptionTitle->setFixedHeight(h_Title);
    mZOptionTitle->setIsTitle(true);

    mZOptionBut = new Button(tr("Optimal Z Display"), mZOptionGroup);
    mZOptionBut->setFixedHeight(h_Button);
    mZOptionBut->setToolTip(tr("Optimize Z scale"));

    mZOptionLab = new QLabel(tr("Z"), mZOptionGroup);
    mZOptionLab->setFixedHeight(h_Label);

    mCurrentZMinEdit = new LineEdit(mZOptionGroup);
    mCurrentZMinEdit->setFixedHeight(h_Edit);
    mCurrentZMinEdit->setToolTip(tr("Enter a minimal value to display the curves"));

    mCurrentZMaxEdit = new LineEdit(mZOptionGroup);
    mCurrentZMaxEdit->setFixedHeight(h_Edit);
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
    mGraphicTitle = new Label(tr("Graphic Options"), mDisplayGroup);
    mGraphicTitle->setFixedHeight(h_Title);
    mGraphicTitle->setIsTitle(true);

    mGraphicGroup = new QWidget();

    mZoomLab = new QLabel(tr("Zoom"), mGraphicGroup);
    mZoomLab->setFixedHeight(h_Label);
    mZoomLab->setAlignment(Qt::AlignCenter);

    mZoomSlider = new QSlider(Qt::Horizontal, mGraphicGroup);
    mZoomSlider->setFixedHeight(h_Slider);
    mZoomSlider->setRange(10, 1000);
    mZoomSlider->setTickInterval(1);
    mZoomSlider->setValue(100);

    mZoomEdit = new LineEdit(mGraphicGroup);
    mZoomEdit->setValidator(RplusValidator);
    mZoomEdit->setToolTip(tr("Enter zoom value to increase graph height"));
    mZoomEdit->setFixedHeight(h_Edit);
    mZoomEdit->setFixedWidth(mOptionsW/3); //for windows new spin box
    mZoomEdit->setText(QLocale().toString(mZoomSlider->value()));

    mLabFont = new QLabel(tr("Font"), mGraphicGroup);
    mLabFont->setFixedHeight(h_Label);
    mLabFont->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mFontBut = new Button(font().family() + ", " + QString::number(font().pointSizeF()), mGraphicGroup);
    mFontBut->setFixedHeight(h_Button);
    mFontBut->setToolTip(tr("Click to change the font on the drawing"));

    mLabThickness = new QLabel(tr("Thickness"), mGraphicGroup);
    mLabThickness->setFixedHeight(h_Label);
    mLabThickness->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mThicknessCombo = new QComboBox(mGraphicGroup);
    mThicknessCombo->setFixedHeight(h_Combo);
    mThicknessCombo->addItem("1 px");
    mThicknessCombo->addItem("2 px");
    mThicknessCombo->addItem("3 px");
    mThicknessCombo->addItem("4 px");
    mThicknessCombo->addItem("5 px");
    mThicknessCombo->setToolTip(tr("Select to change the thickness of the drawing"));
    mThicknessCombo->setCurrentIndex(1);

    mLabOpacity = new QLabel(tr("Opacity"), mGraphicGroup);
    mLabOpacity->setFixedHeight(h_Label);
    mLabOpacity->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mOpacityCombo = new QComboBox(mGraphicGroup);
    mOpacityCombo->setFixedHeight(h_Combo);
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
    connect(mZoomEdit, &LineEdit::editingFinished, this, &ResultsView::applyZoomEdit);
    connect(mFontBut, &QPushButton::clicked, this, &ResultsView::applyFont);
    connect(mThicknessCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ResultsView::applyThickness);
    connect(mOpacityCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ResultsView::applyOpacity);

    QHBoxLayout* graphicLayout1 = new QHBoxLayout();
    graphicLayout1->setContentsMargins(0, 0, 0, 0);
    graphicLayout1->addWidget(mZoomLab);
    graphicLayout1->addWidget(mZoomSlider);
    graphicLayout1->addWidget(mZoomEdit);

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
    mDisplayGroup->setLayout(displayLayout);

    // ------------------------------------
    //  Distrib. Option / MCMC Chains
    //  Note : mChainChecks and mChainRadios are populated by createChainsControls()
    // ------------------------------------
    mChainsTitle = new Label(tr("MCMC Chains"), this);
    mChainsTitle->setFixedHeight(h_Title);
    mChainsTitle->setIsTitle(true);

    mChainsGroup = new QWidget();

    mAllChainsCheck = new CheckBox(tr("Chain Concatenation"), mChainsGroup);
    mAllChainsCheck->setFixedHeight(h_Check);
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
    mDensityOptsTitle->setFixedHeight(h_Title);
    mDensityOptsTitle->setIsTitle(true);

    mDensityOptsGroup = new QWidget();

    mCredibilityCheck = new CheckBox(tr("Show Confidence Bar"));
    mCredibilityCheck->setFixedHeight(h_Check);
    mCredibilityCheck->setChecked(true);

    mThreshLab = new QLabel(tr("Confidence Level (%)"), mDensityOptsGroup);
    mThreshLab->setFixedHeight(h_Label);
    mThreshLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mThresholdEdit = new LineEdit(mDensityOptsGroup);
    mThresholdEdit->setFixedHeight(h_Edit);
    DoubleValidator* percentValidator = new DoubleValidator();
    percentValidator->setBottom(0.0);
    percentValidator->setTop(100.0);
    mThresholdEdit->setValidator(percentValidator);

    mFFTLenLab = new QLabel(tr("Grid Length"), mDensityOptsGroup);
    mFFTLenLab->setFixedHeight(h_Label);
    mFFTLenLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mFFTLenCombo = new QComboBox(mDensityOptsGroup);
    mFFTLenCombo->setFixedHeight(h_Combo);
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

    mFFTLenCombo->setCurrentIndex(5);

    mBandwidthLab = new QLabel(tr("FFTW Bandwidth"), mDensityOptsGroup);
    mBandwidthLab->setFixedHeight(h_Label);
    mBandwidthLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mBandwidthEdit = new LineEdit(mDensityOptsGroup);
    mBandwidthEdit->setFixedHeight(h_Edit);
    mBandwidthEdit->setValidator(RplusValidator);

    connect(mCredibilityCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mFFTLenCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ResultsView::applyFFTLength);
    connect(mBandwidthEdit, &LineEdit::editingFinished, this, &ResultsView::applyBandwidth);
    connect(mThresholdEdit, &LineEdit::editingFinished, this, &ResultsView::applyThreshold);


    // Used with Activity

    mActivityOptsTitle = new Label(tr("Activity Options"), this);
    mActivityOptsTitle->setFixedHeight(h_Title);
    mActivityOptsTitle->setIsTitle(true);

    mActivityOptsGroup  = new QWidget();
    mRangeThreshLab = new QLabel(tr("Time Range Level (%)"), mActivityOptsGroup);
    mRangeThreshLab->setFixedHeight(h_Label);
    mRangeThreshLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mRangeThresholdEdit = new LineEdit(mDensityOptsGroup);
    mRangeThresholdEdit->setFixedHeight(h_Edit);
    mRangeThresholdEdit->setValidator(percentValidator);

    mHActivityLab = new QLabel(tr("Activity Bandwidth"), mActivityOptsGroup);
    mHActivityLab->setFixedHeight(h_Label);

    mHActivityEdit = new LineEdit(mDensityOptsGroup);
    mHActivityEdit->setFixedHeight(h_Edit);
    mHActivityEdit->setValidator(RplusValidator);

    connect(mRangeThresholdEdit, &LineEdit::editingFinished, this, &ResultsView::applyHActivity);
    connect(mHActivityEdit, &LineEdit::editingFinished, this, &ResultsView::applyHActivity);

    QHBoxLayout* densityLayout1 = new QHBoxLayout();
    densityLayout1->addWidget(mFFTLenLab);
    densityLayout1->addWidget(mFFTLenCombo);

    QHBoxLayout* densityLayout2 = new QHBoxLayout();
    densityLayout2->addWidget(mBandwidthLab);
    densityLayout2->addWidget(mBandwidthEdit);

    QHBoxLayout* densityLayout3 = new QHBoxLayout();
    densityLayout3->addWidget(mThreshLab);
    densityLayout3->addWidget(mThresholdEdit);

    QVBoxLayout* densityLayout = new QVBoxLayout();
    densityLayout->setContentsMargins(10, 0, 0, 0);
    densityLayout->setSpacing(5);
    densityLayout->addWidget(mCredibilityCheck);
    densityLayout->addLayout(densityLayout1);
    densityLayout->addLayout(densityLayout2);
    densityLayout->addLayout(densityLayout3);

    mDensityOptsGroup->setLayout(densityLayout);

    QHBoxLayout* activityLayout1 = new QHBoxLayout();
    activityLayout1->addWidget(mRangeThreshLab);
    activityLayout1->addWidget(mRangeThresholdEdit);

    QHBoxLayout* activityLayout2 = new QHBoxLayout();
    activityLayout2->addWidget(mHActivityLab);
    activityLayout2->addWidget(mHActivityEdit);

    QVBoxLayout* activityLayout = new QVBoxLayout();
    activityLayout->setContentsMargins(10, 0, 0, 0);
    activityLayout->setSpacing(5);
    densityLayout->addLayout(activityLayout1);
    densityLayout->addLayout(densityLayout2);
    mActivityOptsGroup->setLayout(activityLayout);
    // ------------------------------------
    //  Tab Distrib. Options
    // ------------------------------------
    mDistribGroup = new QWidget();

    QVBoxLayout* mcmcLayout = new QVBoxLayout();
    mcmcLayout->setContentsMargins(0, 0, 0, 0);
    mcmcLayout->setSpacing(0);
    mcmcLayout->addWidget(mChainsTitle);
    mcmcLayout->addWidget(mChainsGroup);
    mcmcLayout->addWidget(mDensityOptsTitle);
    mcmcLayout->addWidget(mDensityOptsGroup);
    mcmcLayout->addWidget(mActivityOptsTitle);
    mcmcLayout->addWidget(mActivityOptsGroup);
    mDistribGroup->setLayout(mcmcLayout);

    // ------------------------------------
    //  Tab Page
    // ------------------------------------

    mPageSaveGroup = new QWidget();

    const qreal layoutWidth = mOptionsW;
    const qreal internSpacing = 2;

    mPageWidget = new QWidget(mPageSaveGroup);

    mPreviousPageBut = new Button(tr("Prev."), mPageWidget);
    //mPreviousPageBut->setFixedHeight(33);// set after
    mPreviousPageBut->setCheckable(false);
    mPreviousPageBut->setFlatHorizontal();
    mPreviousPageBut->setToolTip(tr("Display previous data"));
    mPreviousPageBut->setIconOnly(false);

    mPageEdit = new LineEdit(mPageWidget);
    //mPageEdit->setFixedHeight(33); // set after
    mPageEdit->setEnabled(false);
    mPageEdit->setReadOnly(true);
    mPageEdit->setAlignment(Qt::AlignCenter);
    mPageEdit->setText(QString::number(mMaximunNumberOfVisibleGraph));

    mNextPageBut = new Button(tr("Next"), mPageWidget);
    //mNextPageBut->setFixedHeight(33); // set after
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
    const qreal layoutWidthBy3 = (layoutWidth - 2*internSpacing)/3;
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

    mSaveAllWidget = new QWidget(mPageSaveGroup);

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
    mSaveSelectWidget = new QWidget(mPageSaveGroup);

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

   /* mImageSaveBut->setFixedHeight(33);
    mImageClipBut->setFixedHeight(33);
    mResultsClipBut->setFixedHeight(33);
    mDataSaveBut->setFixedHeight(33);*/

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
    mPageSaveTab->setFixedHeight(h_Tab);
    mPageSaveTab->addTab(tr("Page"));
    mPageSaveTab->addTab(tr("Saving"));


    mPageSaveTab->setTab(0, false);

    connect(mPageSaveTab, static_cast<void (Tabs::*)(const qsizetype&)>(&Tabs::tabClicked), this, &ResultsView::updateOptionsWidget);

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
    mOptionsLayout->addWidget(mDisplayGroup);

    //mOptionsLayout->addSpacing(2);
    mOptionsLayout->addWidget(mPageSaveTab);
    mOptionsLayout->addWidget(mPageWidget);


    mSaveAllWidget->setVisible(true);
    mSaveSelectWidget->setVisible(true);

    mDistribGroup->setVisible(true);

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
    mCurvesScrollArea->setVisible(false);

    mGraphHeight = 4 * AppSettings::heigthUnit();
    mHeightForVisibleAxis = mGraphHeight ;

    mMarker->raise();

    createOptionsWidget();

}

ResultsView::~ResultsView()
{
    deleteAllGraphsInList(mByEventsGraphs);
    deleteAllGraphsInList(mByPhasesGraphs);
    deleteAllGraphsInList(mByCurvesGraphs);

}


#pragma mark Project & Model


void ResultsView::setProject()
{
    /* Starting MCMC calculation does a mModel.clear() at first, and recreate it.
     * Then, it fills its elements (events, ...) with calculated data (trace, ...)
     * If the process is canceled, we only have unfinished data in storage.
     * => The previous nor the new results can be displayed so we must start by clearing the results view! */

    clearResults();
    initModel();
    connect(getProject_ptr().get(), &Project::mcmcStarted, this, &ResultsView::clearResults);
}

void ResultsView::clearResults()
{
    deleteAllGraphsInList(mByEventsGraphs);
    deleteAllGraphsInList(mByPhasesGraphs);
    deleteAllGraphsInList(mByCurvesGraphs);
    mZoomsT.clear();
    mZoomsX.clear();
    mZoomsY.clear();
    mZoomsZ.clear();
    mCurrentVariableList.clear();
    mScalesT.clear();

}

void ResultsView::updateModel()
{
    createGraphs(); // do deleteAllGraphsInList
    updateLayout();

}

void ResultsView::initModel()
{
    auto model = getModel_ptr();

    mHasPhases = (model->mPhases.size() > 0);

    mGraphListTab->setTabVisible(1, mHasPhases);
    mGraphListTab->setTabVisible(2, isCurve());
    // ----------------------------------------------------
    //  Create Chains option controls (radio and checkboxes under "MCMC Chains")
    // ----------------------------------------------------
    createChainsControls();
    mAllChainsCheck->setChecked(true);

    mCurrentTypeGraph = GraphViewResults::ePostDistrib;
    mCurrentVariableList.clear();

    mRangeThresholdEdit->resetText(95.0);

    mHpdThreshold = model->getThreshold();
    auto threshold_str = stringForLocal(mHpdThreshold) + "%";
    mCurveGRadio->setText(tr("Curve (at %1% Level)").arg(threshold_str));

    mThresholdEdit->resetText(mHpdThreshold);
    mHActivityEdit->resetText(model->mHActivity);

    mFFTLenCombo->setCurrentText(stringForLocal(model->getFFTLength()));
    mBandwidthEdit->resetText(model->getBandwidth());

    mZoomsT.clear();
    mZoomsX.clear();
    mZoomsY.clear();
    mZoomsZ.clear();

    mResultMaxT = model->mSettings.getTmaxFormated();
    mResultMinT = model->mSettings.getTminFormated();
    mResultZoomT = 1;
    mResultCurrentMaxT = model->mSettings.getTmaxFormated();
    mResultCurrentMinT = model->mSettings.getTminFormated();

    applyStudyPeriod();
    updateGraphsMinMax();


    if (isCurve()) {

        mMainVariable = GraphViewResults::eG;
        mCurveGRadio->setChecked(true);
        mGraphListTab->setTab(2, false);

        const auto &gx = model->mPosteriorMeanG.gx;
        const auto minmax_Y = gx.mapG.minMaxY();

        double minY = +std::numeric_limits<double>::max();
        double maxY = -std::numeric_limits<double>::max();
        minY = std::accumulate(model->mEvents.begin(), model->mEvents.end(), minY, [](double x, std::shared_ptr<Event> e) {return std::min(e->mXIncDepth, x);});
        maxY = std::accumulate(model->mEvents.begin(), model->mEvents.end(), maxY, [](double x, std::shared_ptr<Event> e) {return std::max(e->mXIncDepth, x);});
        int i = 0;
        for (const auto &g : gx.vecG) {
            const auto e = 1.96*sqrt(gx.vecVarG.at(i));
            minY = std::min(minY, g - e);
            maxY = std::max(maxY, g + e);
            i++;
        }

        Scale XScale;
        XScale.findOptimal(std::min(minmax_Y.first, minY), std::max(minmax_Y.second, maxY), 7);

        mResultCurrentMinX = XScale.min;
        mResultCurrentMaxX = XScale.max;
        setXRange();

        mXOptionTitle->setText(model->getCurvesLongName().at(0) + " " + tr("Scale"));
        mXOptionLab->setText(model->getCurvesName().at(0));
        mXOptionBut->setText(tr("Optimal") + " " + model->getCurvesName().at(0) + " " + tr("Display"));

        if (model->displayY() && !model->mPosteriorMeanG.gy.vecG.empty() ) {
            const auto &gy = model->mPosteriorMeanG.gy;
            const auto minmax_Y = gy.mapG.minMaxY();

            minY = +std::numeric_limits<double>::max();
            maxY = -std::numeric_limits<double>::max();
            minY = std::accumulate(model->mEvents.begin(), model->mEvents.end(), minY, [](double x, std::shared_ptr<Event> e) {return std::min(e->mYDec, x);});
            maxY = std::accumulate(model->mEvents.begin(), model->mEvents.end(), maxY, [](double x, std::shared_ptr<Event> e) {return std::max(e->mYDec, x);});
            int i = 0;
            for (const auto &g : gy.vecG) {
                const auto e = 1.96*sqrt(gy.vecVarG.at(i));
                minY = std::min(minY, g - e);
                maxY = std::max(maxY, g + e);
                i++;
            }

            XScale.findOptimal(std::min(minmax_Y.first, minY), std::max(minmax_Y.second, maxY), 7);

            mResultCurrentMinY = XScale.min;
            mResultCurrentMaxY = XScale.max;
            setYRange();
            mYOptionTitle->setText(model->getCurvesLongName().at(1) + " " + tr("Scale"));
            mYOptionLab->setText(model->getCurvesName().at(1));
            mYOptionBut->setText(tr("Optimal") + " " + model->getCurvesName().at(1) + " " + tr("Display"));

            if (model->displayZ() && !model->mPosteriorMeanG.gz.vecG.empty() ) {
                const auto &gz = model->mPosteriorMeanG.gz;
                const auto minmax_Y = gz.mapG.minMaxY();

                minY = +std::numeric_limits<double>::max();
                maxY = -std::numeric_limits<double>::max();
                minY = std::accumulate(model->mEvents.begin(), model->mEvents.end(), minY, [](double x, std::shared_ptr<Event> e) {return std::min(e->mZField, x);});
                maxY = std::accumulate(model->mEvents.begin(), model->mEvents.end(), maxY, [](double x, std::shared_ptr<Event> e) {return std::max(e->mZField, x);});
                int i = 0;
                for (const auto &g : gz.vecG) {
                    const auto e = 1.96*sqrt(gz.vecVarG.at(i));
                    minY = std::min(minY, g - e);
                    maxY = std::max(maxY, g + e);
                    i++;
                }


                XScale.findOptimal(std::min(minmax_Y.first, minY), std::max(minmax_Y.second, maxY), 7);

                mResultCurrentMinZ = XScale.min;
                mResultCurrentMaxZ = XScale.max;
                setZRange();
                mZOptionTitle->setText(model->getCurvesLongName().at(2) + " " + tr("Scale"));
                mZOptionLab->setText(model->getCurvesName().at(2));
                mZOptionBut->setText(tr("Optimal") + " " + model->getCurvesName().at(2) + " " + tr("Display"));
            }
        }

    } else if (mHasPhases) {
        mMainVariable = GraphViewResults::eBeginEnd;
        mGraphListTab->setTabVisible(1, true);
        mGraphListTab->setTab(1, false);

    } else {
        mMainVariable = GraphViewResults::eThetaEvent;
        mGraphListTab->setTab(0, false);
    }
    updateMainVariable();
    mCurrentVariableList.append(mMainVariable);

    updateOptionsWidget();
    createGraphs(); // do GraphViewResults::updateLayout == paint
    updateLayout(); // done in showStats() ??

    showStats(mEventsStatCheck->isChecked());
}

void ResultsView::applyAppSettings()
{
    auto model = getModel_ptr();

    auto threshold_str = stringForLocal(model->getThreshold());
    mCurveGRadio->setText(tr("Curve (at %1% Level)").arg(threshold_str));

    mRangeThresholdEdit->setText(stringForLocal(95.0));
    mThresholdEdit->setText(stringForLocal(model->getThreshold()));
    mHActivityEdit->setText(stringForLocal(model->mHActivity));

    mFFTLenCombo->setCurrentText(stringForLocal(model->getFFTLength()));
    mBandwidthEdit->resetText(model->getBandwidth());

    setTimeRange();

    generateCurves();

}

#pragma mark Layout
bool ResultsView::event(QEvent *e)
{
    if (e->type() == QEvent::Wheel &&
        (mCurvesScrollArea->underMouse() || mEventsScrollArea->underMouse() || mPhasesScrollArea->underMouse() )) {
            emit wheelMove(e);
    }
    return QWidget::event(e);
}


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
    const int markerXPos = std::clamp( x, 0, mRuler->x() + mRuler->width());
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
    if (mEventsStatCheck->isChecked() || mPhasesStatCheck->isChecked() || mCurveStatCheck->isChecked()) {
        graphWidth = (2./3.) * leftWidth;
    }

    mGraphTypeTabs->setGeometry(mMargin, mMargin, leftWidth, tabsH);
    mRuler->setGeometry(mMargin, mMargin + tabsH, graphWidth, Ruler::sHeight);

    const int stackH = height() - mMargin - tabsH - Ruler::sHeight;
    const QRect graphScrollGeometry(mMargin, mMargin + tabsH + Ruler::sHeight, leftWidth, stackH);

    mEventsScrollArea->setGeometry(graphScrollGeometry);
    mPhasesScrollArea->setGeometry(graphScrollGeometry);
    mCurvesScrollArea->setGeometry(graphScrollGeometry);

    updateGraphsLayout();
    updateMarkerGeometry(mMarker->pos().x());
}


void ResultsView::updateGraphsLayout()
{
    // Display the scroll area corresponding to the selected tab
    mEventsScrollArea->setVisible(mGraphListTab->currentIndex() == 0);
    mPhasesScrollArea->setVisible(mGraphListTab->currentIndex() == 1);
    mCurvesScrollArea->setVisible(mGraphListTab->currentIndex() == 2);

    if (mGraphListTab->currentIndex() == 0) {
        updateGraphsLayout(mEventsScrollArea, mByEventsGraphs);

    } else if (mGraphListTab->currentIndex() == 1) {
        updateGraphsLayout(mPhasesScrollArea, mByPhasesGraphs);

    } else if (mGraphListTab->currentIndex() == 2) {
        updateGraphsLayout(mCurvesScrollArea, mByCurvesGraphs);
    }

}

void ResultsView::updateGraphsLayout(QScrollArea* scrollArea, QList<GraphViewResults*> graphs)
{
    QWidget* widget = scrollArea->widget();

    // Ensure the widget is smaller than the scrollArea to prevent horizontal scrolling,
    // while allowing the graphs inside the widget to have the same size
    if (widget) {
        // Resize the widget to fit the scroll area width and the total height of all graphs.
        widget->resize(scrollArea->width() - 2, static_cast<int>(graphs.size()) * mGraphHeight);

        // Position each graph within the widget.
        int i = 0;
        for (GraphViewResults* g : graphs) {
            g->setGeometry(0, i * mGraphHeight, scrollArea->width(), mGraphHeight);
            g->setVisible(true);
            i++;
            // Uncomment the line below if you need to update the graph's display.
            // g->update();
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
    mCurrentTypeGraph = (GraphViewResults::graph_t) mGraphTypeTabs->currentIndex();

    //createGraphs();

    updateOptionsWidget();
    if (mGraphListTab->currentName() == tr("Events")) {
        createByEventsGraphs();

    } else if (mGraphListTab->currentName() == tr("Phases")) {
        createByPhasesGraphs();

    } else if (mGraphListTab->currentName() == tr("Curves")) {
        createByCurveGraph();
    }
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
    int currentIndex = static_cast<int>(mGraphListTab->currentIndex());

    // Display the scroll area corresponding to the selected tab.
    mEventsScrollArea->setVisible(currentIndex == 0);
    mPhasesScrollArea->setVisible(currentIndex == 1);
    mCurvesScrollArea->setVisible(currentIndex == 2);

    // Update the current variable based on the selected tab.
    switch (currentIndex) {
    case 0:
        mMainVariable = GraphViewResults::eThetaEvent;
        mEventThetaRadio->setChecked(true);
        break;

    case 1:
        mMainVariable = GraphViewResults::eBeginEnd;
        mBeginEndRadio->setChecked(true);
        break;

    case 2:
        mMainVariable = GraphViewResults::eG;
        mCurveGRadio->setChecked(true);
        break;

    default:
        // Handle unexpected index if necessary
        break;
    }

    // Set the current graph type to Posterior distribution.
    mGraphTypeTabs->setTab(0, false);
    mCurrentTypeGraph = static_cast<GraphViewResults::graph_t>(mGraphTypeTabs->currentIndex());

    // Changing the graphs list implies going back to page 1.
    mCurrentPage = 0;

    applyCurrentVariable();
}


void ResultsView::updateMainVariable()
{
    mCurrentVariableList.clear();

    // Check the current tab name and update the main variable accordingly.
    QString currentTabName = mGraphListTab->currentName();

    if (currentTabName == tr("Events")) {
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

#ifdef S02_BAYESIAN
        } else if (mS02Radio->isChecked()) {
            mMainVariable = GraphViewResults::eS02;


#endif
        } else if (mEventVGRadio->isChecked()) {
            mMainVariable = GraphViewResults::eVg;
        }

    } else if (currentTabName == tr("Phases")) {

        if (mBeginEndRadio->isChecked()) {
            mMainVariable = GraphViewResults::eBeginEnd;

            if (mPhasesEventsUnfoldCheck->isChecked()) {
                mCurrentVariableList.append(GraphViewResults::eThetaEvent);

                if (mPhasesDatesUnfoldCheck->isChecked()) {
                    mCurrentVariableList.append(GraphViewResults::eDataTi);

                }
            }


        }  else if (mTempoRadio->isChecked()) {
            mMainVariable = GraphViewResults::eTempo;

            if (mPhasesEventsUnfoldCheck->isChecked()) {
                mCurrentVariableList.append(GraphViewResults::eThetaEvent);

                if (mPhasesDatesUnfoldCheck->isChecked()) {
                    mCurrentVariableList.append(GraphViewResults::eDataTi);

                }
              }


        } else  if (mActivityRadio->isChecked()) {
            mMainVariable = GraphViewResults::eActivity;

            if (mPhasesEventsUnfoldCheck->isChecked()) {
                mCurrentVariableList.append(GraphViewResults::eThetaEvent);

                if (mPhasesDatesUnfoldCheck->isChecked()) {
                    mCurrentVariableList.append(GraphViewResults::eDataTi);

                }
            }

        } else  if (mDurationRadio->isChecked()) {
            mMainVariable = GraphViewResults::eDuration;

        }

    } else if (currentTabName == tr("Curves")) {

        if (mLambdaRadio->isChecked()) {
            mMainVariable = GraphViewResults::eLambda;
#ifdef KOMLAN
        } else if (mS02VgRadio->isChecked()) {
            mMainVariable = GraphViewResults::eS02Vg;
#endif
        } else if (mCurveGRadio->isChecked()) {
            mMainVariable = GraphViewResults::eG;
            if (mCurveErrorCheck->isChecked())
                mCurrentVariableList.append(GraphViewResults::eGGauss);

            if (mCurveHpdCheck->isChecked())
                mCurrentVariableList.append(GraphViewResults::eGHpd);

            if (mCurveMapCheck->isChecked())
                mCurrentVariableList.append(GraphViewResults::eMap);

            if (mCurveEventsPointsCheck->isChecked())
                mCurrentVariableList.append(GraphViewResults::eGEventsPts);

            if (mCurveDataPointsCheck->isChecked())
                mCurrentVariableList.append(GraphViewResults::eGDatesPts);

        } else if (mCurveGPRadio->isChecked()) {
            mMainVariable = GraphViewResults::eGP;

            if (mCurveGPHpdCheck->isChecked())
                mCurrentVariableList.append(GraphViewResults::eGPHpd);

            if (mCurveGPMapCheck->isChecked())
                mCurrentVariableList.append(GraphViewResults::eGPMap);

            if (mCurveGPGaussCheck->isChecked())
                mCurrentVariableList.append(GraphViewResults::eGP);


        } else if (mCurveGSRadio->isChecked()) {
            mMainVariable = GraphViewResults::eGS;
        }

    }
    // Set the current graph type to Posterior distribution.
    mCurrentTypeGraph = GraphViewResults::ePostDistrib;
    mGraphTypeTabs->setTab(0, false);

    // Append the main variable to the current variable list.
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
// useless
void ResultsView::toggleDisplayDistrib()
{
    auto model = getModel_ptr();
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

    const bool displayX = model->displayX();
    const bool displayY = model->displayY();
    const bool displayZ = model->displayZ();
    // Search for the visible widget
    QWidget* widFrom = nullptr;
    int widHeigth = 0;

    if (mDisplayGroup->isVisible())
        widFrom = mDisplayGroup;

    else if (mDistribGroup->isVisible())
        widFrom = mDistribGroup;

    // Exchange with the widget corresponding to the requested tab
    if (mDisplayDistribTab->currentName() == tr("Display") ) {
        mDisplayGroup->setVisible(true);
        mDistribGroup->setVisible(false);
        // ------------------------------------
        //  Display Options
        // ------------------------------------
        const  int internSpan = 5;
        const int h = mDisplayStudyBut->height();

        widHeigth = 11*h + 12*internSpan;

        mDisplayStudyBut->setText(xScaleRepresentsTime() ? tr("Study Period Display") : tr("Fit Display"));
        mDisplayStudyBut->setVisible(true);
        widHeigth += mDisplayStudyBut->height() + internSpan;


        if (widFrom != mDisplayGroup)
            mOptionsLayout->replaceWidget(widFrom, mDisplayGroup);

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
            mDisplayGroup->setFixedHeight(widHeigth + (displayZ ? mZOptionGroup->height() + mZOptionTitle->height(): 0.)) ;

        } else {
            mXOptionTitle->setVisible(false);
            mXOptionGroup->setVisible(false);

            mYOptionTitle->setVisible(false);
            mYOptionGroup->setVisible(false);

            mYOptionTitle->setVisible(false);
            mYOptionGroup->setVisible(false);
            mDisplayGroup-> setFixedHeight(widHeigth);
        }

    } else { // Tab Distrib. Option
        mDisplayGroup->setVisible(false);
        mDistribGroup->setVisible(true);
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
                                              + mBandwidthEdit->height() + mHActivityEdit->height() + mRangeThresholdEdit->height()
                                               + 12* internSpan);

            widHeigth += mDensityOptsTitle->height() + mDensityOptsGroup->height();

        }
        widHeigth += 4*internSpan;
        mDistribGroup-> setFixedHeight(widHeigth);
        if (widFrom != mDistribGroup)
            mOptionsLayout->replaceWidget(widFrom, mDistribGroup);
    }

}

void ResultsView::createChainsControls()
{
    auto model =getModel_ptr();
    if (model->mChains.size() != (size_t)mChainChecks.size()) {
        deleteChainsControls();

        for (size_t i=0; i<model->mChains.size(); ++i) {
            CheckBox* check = new CheckBox(tr("Chain %1").arg(QString::number(i+1)), mChainsGroup);
            check->setFixedHeight(h_Check);
            check->setVisible(true);
            mChainChecks.append(check);

            connect(check, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);

            RadioButton* radio = new RadioButton(tr("Chain %1").arg(QString::number(i+1)), mChainsGroup);
            radio->setFixedHeight(h_Radio);
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
        check = nullptr;
    }
    mChainChecks.clear();

    for (RadioButton*& radio : mChainRadios) {
        disconnect(radio, &RadioButton::clicked, this, &ResultsView::updateCurvesToShow);
        delete radio;
        radio = nullptr;
    }
    mChainRadios.clear();
}

#pragma mark Graphs UI

void ResultsView::createGraphs()
{
    if (getModel_ptr() == nullptr) {
        return;
    }
    updateMainVariable();

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
    auto model = getModel_ptr();
    if (model == nullptr) {
        mMaximunNumberOfVisibleGraph = 0;
        return;
    }
    
    int totalGraphs = 0;
    
    if (mGraphListTab->currentName() == tr("Events")) {
        bool showAllEvents = ! model->hasSelectedEvents();
        for (const auto& event : model->mEvents) {
            if (event->mIsSelected || showAllEvents) {
                ++totalGraphs;
                
                if (mEventsDatesUnfoldCheck->isChecked())
                    totalGraphs += event->mDates.size();

            }
        }
    } else if (mGraphListTab->currentName() == tr("Phases")) {
        bool showAllPhases = ! model->hasSelectedPhases();

        for (const auto& phase : model->mPhases) {
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
#ifdef KOMLAN
        } else if (mS02VgRadio->isChecked()) {
                ++totalGraphs;
#endif
        } else {
            if (!model->mEvents.empty() && isCurve()) {
                ++totalGraphs;
                if (model->displayY()) ++totalGraphs;
                if (model->displayZ()) ++totalGraphs;
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
    auto model = getModel_ptr();
    if (model == nullptr) {
        mMaximunNumberOfVisibleGraph = 0;
        return;
    }

    // ----------------------------------------------------------------------
    //  Disconnect and delete existing graphs
    // ----------------------------------------------------------------------
    deleteAllGraphsInList(mByEventsGraphs);

    // ----------------------------------------------------------------------
    // Show all events unless at least one is selected
    // ----------------------------------------------------------------------
    bool showAllEvents = ! model->hasSelectedEvents();

    // ----------------------------------------------------------------------
    //  Iterate through all events and create corresponding graphs
    // ----------------------------------------------------------------------
    int graphIndex = 0;

    blockSignals(true);
    for (const auto& event : model->mEvents) {
        if (event->mIsSelected || showAllEvents) {
            if (graphIndexIsInCurrentPage(graphIndex)) {
                GraphViewEvent* graph = new GraphViewEvent(mEventsWidget);
                graph->setEvent(event);

                setGraphicOption(*graph);

                mByEventsGraphs.append(graph);
                connect(graph, &GraphViewResults::selected, this, &ResultsView::updateOptionsWidget);
            }
            ++graphIndex;
                
            if (mEventsDatesUnfoldCheck->isChecked()) {
                for (auto&& date : event->mDates) {
                    if (graphIndexIsInCurrentPage(graphIndex)) {
                        GraphViewDate* graph = new GraphViewDate(mEventsWidget);
                        graph->setDate(&date);
                        graph->setItemColor(event->mColor);
                        setGraphicOption(*graph);

                        mByEventsGraphs.append(graph);
                        connect(graph, &GraphViewResults::selected, this, &ResultsView::updateOptionsWidget);
                    }
                    ++graphIndex;
                }
            }

        }
    }
    blockSignals(false);
}

void ResultsView::createByPhasesGraphs()
{
    auto model = getModel_ptr();
    if (model == nullptr) {
        mMaximunNumberOfVisibleGraph = 0;
        return;
    }

    // ----------------------------------------------------------------------
    // Show all, unless at least one is selected
    // ----------------------------------------------------------------------
    const bool showAllPhases = ! model->hasSelectedPhases();

    // ----------------------------------------------------------------------
    //  Disconnect and delete existing graphs
    // ----------------------------------------------------------------------
    deleteAllGraphsInList(mByPhasesGraphs);

    // ----------------------------------------------------------------------
    //  Iterate through all, and create corresponding graphs
    // ----------------------------------------------------------------------
    int graphIndex = 0;

    blockSignals(true);
    for (const auto& phase : model->mPhases) {
        if (phase->mIsSelected || showAllPhases) {
            if (graphIndexIsInCurrentPage(graphIndex)) {
                GraphViewPhase* graph = new GraphViewPhase(mPhasesWidget);
                graph->setPhase(phase);

                setGraphicOption(*graph);

                mByPhasesGraphs.append(graph);
                connect(graph, &GraphViewResults::selected, this, &ResultsView::updateOptionsWidget);
            }
            ++graphIndex;
            
            if (mPhasesEventsUnfoldCheck->isChecked()) {
                for (const auto& event : phase->mEvents) {
                     if (graphIndexIsInCurrentPage(graphIndex)) {
                        GraphViewEvent* graph = new GraphViewEvent(mPhasesWidget);
                        graph->setEvent(event);

                        setGraphicOption(*graph);

                        mByPhasesGraphs.append(graph);
                        connect(graph, &GraphViewResults::selected, this, &ResultsView::updateOptionsWidget);
                    }
                    ++graphIndex;
                        
                    if (mPhasesDatesUnfoldCheck->isChecked()) {
                        blockSignals(true);
                        for (auto& date : event->mDates) {                            
                            if (graphIndexIsInCurrentPage(graphIndex))  {
                                GraphViewDate* graph = new GraphViewDate(mPhasesWidget);
                                graph->setDate(&date);
                                graph->setItemColor(event->mColor);

                                setGraphicOption(*graph);

                                //phaseLayout->addWidget(graph);

                                mByPhasesGraphs.append(graph);
                                connect(graph, &GraphViewResults::selected, this, &ResultsView::updateOptionsWidget);
                            }
                            ++graphIndex;
                        }
                        blockSignals(false);
                    }
                }
            }
        }
    }
    blockSignals(false);
}

void ResultsView::createByCurveGraph()
{
    if (!isCurve())
        return;

    auto model = getModel_ptr();
    if (model == nullptr) {
        return;
    }
    // Quantile normal pour 1 - alpha/2
    // 95% envelope  https://en.wikipedia.org/wiki/1.96
    const double threshold = QLocale().toDouble( mThresholdEdit->text());
    const double z_score = zScore(1.0 - threshold * 0.01); // Pour 95% z = 1.96
    // ----------------------------------------------------------------------
    // Show all events unless at least one is selected
    // ----------------------------------------------------------------------
    bool showAllEvents = ! model->hasSelectedEvents();

    // ----------------------------------------------------------------------
    //  Disconnect and delete existing graphs
    // ----------------------------------------------------------------------
    deleteAllGraphsInList(mByCurvesGraphs);

    blockSignals(true);
    if (mLambdaRadio->isChecked())  {
        GraphViewLambda* graphAlpha = new GraphViewLambda(mCurvesWidget);
        setGraphicOption(*graphAlpha);

        graphAlpha->setTitle(tr("Smoothing"));

        mByCurvesGraphs.append(graphAlpha);
        connect(graphAlpha, &GraphViewResults::selected, this, &ResultsView::updateOptionsWidget);

    }
#ifdef KOMLAN
    else if (mS02VgRadio->isChecked())  {
        GraphViewS02Vg* graphS02Vg = new GraphViewS02Vg(mCurvesWidget);
        setGraphicOption(*graphS02Vg);

        graphS02Vg->setTitle(tr("S02Vg"));

        mByCurvesGraphs.append(graphS02Vg);
        connect(graphS02Vg, &GraphViewResults::selected, this, &ResultsView::updateOptionsWidget);

    }
#endif


    else {
        const bool displayY = model->displayY();
        const bool displayZ = model->displayZ();

        // insert refpoints for X
        //  const double thresh = 68.4;

        double pt_Ymin(0.0), pt_Ymax(0.0);
        QList<CurveRefPts> eventsPts;
        QList<CurveRefPts> dataPts;
        // Stock the number of ref points per Event and Data
        std::vector<int> dataPerEvent;
        std::vector<int> hpdPerEvent;
        if (mMainVariable == GraphViewResults::eG) {

            //Same calcul within MultiCalibrationView::scatterPlot(const double thres)
            // Creation des points de ref
            for (const auto& event : model->mEvents) {
                if (event->mIsSelected || showAllEvents) {
                    CurveRefPts evPts;
                    CurveRefPts dPts;
                    double verr;
                    // Set Y for graph X

                    switch (model->mCurveSettings.mProcessType) {
                    case CurveSettings::eProcess_Inclination :
                        verr = event->mS_XA95Depth / 2.448;
                        pt_Ymin = event->mXIncDepth - z_score * verr;
                        pt_Ymax = event->mXIncDepth + z_score * verr;
                        break;
                    case CurveSettings::eProcess_Declination :
                        verr = (event->mS_XA95Depth/2.448) / cos(event->mXIncDepth * M_PI /180.);
                        pt_Ymin = event->mYDec - z_score * verr;
                        pt_Ymax = event->mYDec + z_score * verr;
                        break;
                    case CurveSettings::eProcess_Field :
                        pt_Ymin = event->mZField - z_score * event->mS_ZField;
                        pt_Ymax = event->mZField + z_score * event->mS_ZField;
                        break;
                    case CurveSettings::eProcess_Depth :
                        pt_Ymin = event->mXIncDepth - z_score * event->mS_XA95Depth;
                        pt_Ymax = event->mXIncDepth + z_score * event->mS_XA95Depth;
                        break;

                    case CurveSettings::eProcess_3D:
                    case CurveSettings::eProcess_2D:
                    case CurveSettings::eProcess_Univariate :
                    case CurveSettings::eProcess_Unknwon_Dec:
                        pt_Ymin = event->mXIncDepth - z_score * event->mS_XA95Depth;
                        pt_Ymax = event->mXIncDepth + z_score * event->mS_XA95Depth;
                        break;

                    case CurveSettings::eProcess_Spherical:
                    case CurveSettings::eProcess_Vector:
                        verr = event->mS_XA95Depth / 2.448;
                        pt_Ymin = event->mXIncDepth - z_score * verr;
                        pt_Ymax = event->mXIncDepth + z_score * verr;
                        break;

                    case CurveSettings::eProcess_None:
                    default:
                        break;

                    }
                    evPts.color = event->mColor;

                    // Set X = time

                    if (event->mType == Event::eDefault) {
                        int nb_dataPts = 0;
                        for (const auto& date: event->mDates) {
                            // --- Calibration Date

                            const std::map<double, double> &calibMap = date.getRawCalibMap();
                            // hpd is calculate only on the study Period

                            // hpd results
                            QList<QPair<double, QPair<double, double> > > intervals ;
                            create_HPD_by_dichotomy(calibMap, intervals, date.mTi.mThresholdUsed);
                            // -- Post Distrib of Ti

                            if (intervals.size() > 1) {
                                for (const auto& h : std::as_const(intervals)) {
                                    dPts.Xmin = h.second.first;
                                    dPts.Xmax = h.second.second;
                                    dPts.Ymin = pt_Ymin;
                                    dPts.Ymax = pt_Ymax;
                                    dPts.color = event->mColor;
                                    dPts.type = CurveRefPts::eLine;
                                    // memo Data Points
                                    nb_dataPts++;
                                    dataPts.append(dPts);
                                }
                                dPts.Xmin = intervals.first().second.first;
                                dPts.Xmax = intervals.last().second.second;
                                dPts.Ymin = pt_Ymin;
                                dPts.Ymax = pt_Ymax;
                                dPts.color = event->mColor;
                                dPts.type = CurveRefPts::eDotLineCross;
                                dPts.comment = event->getQStringName();
                                // memo Data Points
                                nb_dataPts++;
                                dataPts.append(dPts);


                            } else if (intervals.size() == 1) {

                                dPts.Xmin = intervals.first().second.first;
                                dPts.Xmax = intervals.last().second.second;
                                dPts.Ymin = pt_Ymin;
                                dPts.Ymax = pt_Ymax;
                                dPts.color = event->mColor;
                                dPts.type = CurveRefPts::eCross;
                                dPts.comment = event->getQStringName();
                                // memo Data Points
                                nb_dataPts++;
                                dataPts.append(dPts);
                            }

                        }
                        dataPerEvent.push_back(nb_dataPts);

                        if (event->mTheta.mSamplerProposal == MHVariable::eFixe) {
                            evPts.Xmin =  event->mTheta.mRawTrace->at(0);
                            evPts.Xmax =  event->mTheta.mRawTrace->at(0);
                            evPts.Ymin = pt_Ymin;
                            evPts.Ymax = pt_Ymax;
                            evPts.color = event->mColor;
                            evPts.type = CurveRefPts::eRoundLine;;
                            evPts.comment = event->getQStringName();
                            // memo Data Points
                            eventsPts.append(evPts);
                            hpdPerEvent.push_back(1);

                        } else {

                            const QList<QPair<double, QPair<double, double> > > &intervals = event->mTheta.mRawHPDintervals;

                            for (const auto& h : intervals) {
                                evPts.Xmin =  h.second.first;
                                evPts.Xmax =  h.second.second;
                                evPts.Ymin = pt_Ymin;
                                evPts.Ymax = pt_Ymax;
                                evPts.color = event->mColor;
                                evPts.type = CurveRefPts::eLine;
                                evPts.comment = event->getQStringName();
                                // memo Data Points
                                eventsPts.append(evPts);
                            }

                            if (intervals.size() > 1) {
                                evPts.Xmin = intervals.first().second.first;
                                evPts.Xmax = intervals.last().second.second;
                                evPts.Ymin = pt_Ymin;
                                evPts.Ymax = pt_Ymax;
                                evPts.color = event->mColor;
                                evPts.type = CurveRefPts::eDotLine;
                                evPts.comment = event->getQStringName();
                                // memo Data Points
                                eventsPts.append(evPts);

                                hpdPerEvent.push_back((int)intervals.size() + 1);

                            } else if (intervals.size() == 1) {
                                hpdPerEvent.push_back(1);
                            }
                        }

                    } else {

                        evPts.Xmin = static_cast<Bound*>(event.get())->mFixed;
                        evPts.Xmax = static_cast<Bound*>(event.get())->mFixed;
                        evPts.Ymin = pt_Ymin;
                        evPts.Ymax = pt_Ymax;
                        evPts.type = CurveRefPts::ePoint;
                        evPts.color = event->mColor;
                        evPts.comment = event->getQStringName();

                        dPts.Xmin = evPts.Xmin;//event->mTheta.mX; // always the same value
                        dPts.Xmax = evPts.Xmax;

                        dPts.Ymin = pt_Ymin;
                        dPts.Ymax = pt_Ymax;
                        dPts.color = event->mColor;
                        dPts.type = CurveRefPts::eRoundLine;
                        dPts.comment = event->getQStringName();

                        // memo Data Points
                        dataPts.append(dPts);
                        eventsPts.append(evPts);
                        hpdPerEvent.push_back(1);
                        dataPerEvent.push_back(1);
                    }
                }
            }
            // End of ref point creation
        }

        GraphViewCurve* graphX = new GraphViewCurve(mCurvesWidget);

        setGraphicOption(*graphX);

        QString resultsHTML = ModelUtilities::curveResultsHTML(model);
        graphX->setNumericalResults(resultsHTML);

        const QStringList curveLongName = model->getCurvesLongName();
        const QString varRateText = tr("Variation Rate");
        const QString accelarationText = tr("Acceleration");

        const QString curveTitleX = curveLongName.at(0) ;

        if (mMainVariable == GraphViewResults::eGP) {
           graphX->setTitle(curveTitleX + " " + varRateText);

        } else if (mMainVariable == GraphViewResults::eGS) {
            graphX->setTitle(curveTitleX + " " + accelarationText);

        } else {
            graphX->setTitle(curveTitleX);
        }

        graphX->setComposanteG(model->mPosteriorMeanG.gx);
        graphX->setComposanteGChains(model->getChainsMeanGComposanteX());

        if (mMainVariable == GraphViewResults::eG) {
            graphX->setEventsPoints(eventsPts);
            graphX->setDataPoints(dataPts);            
        }

        mByCurvesGraphs.append(graphX);

        connect(graphX, &GraphViewResults::selected, this, &ResultsView::updateOptionsWidget);

        
        if (displayY) {

            GraphViewCurve* graphY = new GraphViewCurve(mCurvesWidget);

            setGraphicOption(*graphY);

            const QString curveTitleY = curveLongName.at(1) ;

            if (mMainVariable == GraphViewResults::eGP) {
               graphY->setTitle(curveTitleY + " " + varRateText);

            } else if (mMainVariable == GraphViewResults::eGS) {
                graphY->setTitle(curveTitleY + " " + accelarationText);

            } else {
                graphY->setTitle(curveTitleY);
            }

            graphY->setComposanteG(model->mPosteriorMeanG.gy);
            graphY->setComposanteGChains(model->getChainsMeanGComposanteY());

            if (mMainVariable == GraphViewResults::eG) {
                // change the values of the Y and the error, with the values of the declination and the error, we keep tmean
                int i = 0;
                int iDataPts = 0;
                int iEventPts = -1;
                for (const auto& event : model->mEvents) {
                    if (event->mIsSelected || showAllEvents) {
                        for (auto j = 0 ; j< hpdPerEvent[i]; j++) {
                            iEventPts++;
                            if ( model->mCurveSettings.mProcessType == CurveSettings::eProcess_3D ||
                                 model->mCurveSettings.mProcessType == CurveSettings::eProcess_2D ) {
                                eventsPts[iEventPts].Ymin = event->mYDec;
                                eventsPts[iEventPts].Ymax = event->mYDec;

                            } else if ( model->mCurveSettings.mProcessType == CurveSettings::eProcess_Unknwon_Dec ) {
                                eventsPts[iEventPts].Ymin = event->mZField;
                                eventsPts[iEventPts].Ymax = event->mZField;

                            } else {
                                eventsPts[iEventPts].Ymin = event-> mYDec;
                                eventsPts[iEventPts].Ymax = event-> mYDec;
                            }

                        }

                        try {
                            for (auto j = 0 ; j< dataPerEvent[i]; j++) {

                                if ( model->mCurveSettings.mProcessType == CurveSettings::eProcess_3D ||
                                    model->mCurveSettings.mProcessType == CurveSettings::eProcess_2D ) {
                                    dataPts[iDataPts].Ymin = event->mYDec - z_score * event->mS_Y;
                                    dataPts[iDataPts].Ymax = event->mYDec + z_score * event->mS_Y;

                                } else if ( model->mCurveSettings.mProcessType == CurveSettings::eProcess_Unknwon_Dec ) {
                                    dataPts[iDataPts].Ymin = event->mZField - z_score * event->mS_ZField;
                                    dataPts[iDataPts].Ymax = event->mZField + z_score * event->mS_ZField;

                                } else {
                                    dataPts[iDataPts].Ymin = event-> mYDec - z_score * (event->mS_XA95Depth/2.448) / cos(event->mXIncDepth * M_PI /180.);
                                    dataPts[iDataPts].Ymax = event-> mYDec + z_score * (event->mS_XA95Depth /2.448)/ cos(event->mXIncDepth * M_PI /180.);
                                }
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
            mByCurvesGraphs.append(graphY);

            connect(graphY, &GraphViewResults::selected, this, &ResultsView::updateOptionsWidget);
        }
        
        if (displayZ) {
            GraphViewCurve* graphZ = new GraphViewCurve(mCurvesWidget);

            setGraphicOption(*graphZ);

            const QString curveTitleZ = curveLongName.at(2) ;

            if (mMainVariable == GraphViewResults::eGP) {
               graphZ->setTitle(curveTitleZ + " " + varRateText);

            } else if (mMainVariable == GraphViewResults::eGS) {
                graphZ->setTitle(curveTitleZ + " " + accelarationText);

            } else {
                graphZ->setTitle(curveTitleZ);
            }

            graphZ->setComposanteG(model->mPosteriorMeanG.gz);
            graphZ->setComposanteGChains(model->getChainsMeanGComposanteZ());

            if (mMainVariable == GraphViewResults::eG) {
                int i = 0;
                int iDataPts = 0;
                int iEventPts = -1;
                for (const auto& event : model->mEvents) {
                    if (event->mIsSelected || showAllEvents) {
                        for (int j = 0 ; j< hpdPerEvent[i]; j++) {
                            iEventPts++;
                            eventsPts[iEventPts].Ymin = event->mZField;
                            eventsPts[iEventPts].Ymax = event->mZField;
                        }

                        for (int j = 0 ; j< dataPerEvent[i]; j++) {
                            dataPts[iDataPts].Ymin = event->mZField - z_score * event->mS_ZField;
                            dataPts[iDataPts].Ymax = event->mZField + z_score * event->mS_ZField;
                            iDataPts++;
                        }
                        ++i;
                    }
                }
                graphZ->setEventsPoints(eventsPts);
                graphZ->setDataPoints(dataPts);
            }
            mByCurvesGraphs.append(graphZ);

            connect(graphZ, &GraphViewResults::selected, this, &ResultsView::updateOptionsWidget);
        }
    }
    blockSignals(false);
}

void ResultsView::updateCurveEventsPointX()
{
    if (!isCurve())
        return;

    auto model = getModel_ptr();
    if (model == nullptr) {
        return;
    }
    const double threshold = mThresholdEdit->text().toDouble();
    const double z_score = zScore(1.0 - threshold * 0.01); // Pour 95% z = 1.96
    // ----------------------------------------------------------------------
    // Show all events unless at least one is selected
    // ----------------------------------------------------------------------
    bool showAllEvents = ! model->hasSelectedEvents();


    double pt_Ymin(0.0), pt_Ymax(0.0);
    QList<CurveRefPts> eventsPts;
    QList<CurveRefPts> dataPts;
    // Stock the number of ref points per Event and Data
    std::vector<int> dataPerEvent;
    std::vector<int> hpdPerEvent;
    if (mMainVariable == GraphViewResults::eG) {
        //Same calcul within MultiCalibrationView::scatterPlot(const double thres)
        // Creation des points de ref
        for (const auto& event : model->mEvents) {
            if (event->mIsSelected || showAllEvents) {
                CurveRefPts evPts;
                CurveRefPts dPts;
                double verr;
                // Set Y for graph X

                switch (model->mCurveSettings.mProcessType) {
                case CurveSettings::eProcess_Inclination :
                    verr = event->mS_XA95Depth / 2.448;
                    pt_Ymin = event->mXIncDepth - z_score * verr;
                    pt_Ymax = event->mXIncDepth + z_score * verr;
                    break;
                case CurveSettings::eProcess_Declination :
                    verr = (event->mS_XA95Depth/2.448) / cos(event->mXIncDepth * M_PI /180.);
                    pt_Ymin = event->mYDec - z_score * verr;
                    pt_Ymax = event->mYDec + z_score * verr;
                    break;
                case CurveSettings::eProcess_Field :
                    pt_Ymin = event->mZField - z_score * event->mS_ZField;
                    pt_Ymax = event->mZField + z_score * event->mS_ZField;
                    break;
                case CurveSettings::eProcess_Depth :
                    pt_Ymin = event->mXIncDepth - z_score * event->mS_XA95Depth;
                    pt_Ymax = event->mXIncDepth + z_score * event->mS_XA95Depth;
                    break;

                case CurveSettings::eProcess_3D:
                case CurveSettings::eProcess_2D:
                case CurveSettings::eProcess_Univariate :
                case CurveSettings::eProcess_Unknwon_Dec:
                    pt_Ymin = event->mXIncDepth - z_score * event->mS_XA95Depth;
                    pt_Ymax = event->mXIncDepth + z_score * event->mS_XA95Depth;
                    break;

                case CurveSettings::eProcess_Spherical:
                case CurveSettings::eProcess_Vector:
                    verr = event->mS_XA95Depth / 2.448;
                    pt_Ymin = event->mXIncDepth - z_score * verr;
                    pt_Ymax = event->mXIncDepth + z_score * verr;
                    break;

                case CurveSettings::eProcess_None:
                default:
                    break;

                }
                evPts.color = event->mColor;

                // Set X = time

                if (event->mType == Event::eDefault) {
                    int nb_dataPts = 0;
                    for (const auto& date: event->mDates) {
                        // --- Calibration Date

                        const std::map<double, double> &calibMap = date.getRawCalibMap();
                        // hpd is calculate only on the study Period

                        // hpd results
                        QList<QPair<double, QPair<double, double> > > intervals ;
                        create_HPD_by_dichotomy(calibMap, intervals, date.mTi.mThresholdUsed);
                        // -- Post Distrib of Ti

                        if (intervals.size() > 1) {
                            for (const auto& h : std::as_const(intervals)) {
                                dPts.Xmin = h.second.first;
                                dPts.Xmax = h.second.second;
                                dPts.Ymin = pt_Ymin;
                                dPts.Ymax = pt_Ymax;
                                dPts.color = event->mColor;
                                dPts.type = CurveRefPts::eLine;
                                // memo Data Points
                                nb_dataPts++;
                                dataPts.append(dPts);
                            }
                            dPts.Xmin = intervals.first().second.first;
                            dPts.Xmax = intervals.last().second.second;
                            dPts.Ymin = pt_Ymin;
                            dPts.Ymax = pt_Ymax;
                            dPts.color = event->mColor;
                            dPts.type = CurveRefPts::eDotLineCross;
                            dPts.comment = event->getQStringName();
                            // memo Data Points
                            nb_dataPts++;
                            dataPts.append(dPts);


                        } else if (intervals.size() == 1) {

                            dPts.Xmin = intervals.first().second.first;
                            dPts.Xmax = intervals.last().second.second;
                            dPts.Ymin = pt_Ymin;
                            dPts.Ymax = pt_Ymax;
                            dPts.color = event->mColor;
                            dPts.type = CurveRefPts::eCross;
                            dPts.comment = event->getQStringName();
                            // memo Data Points
                            nb_dataPts++;
                            dataPts.append(dPts);
                        }

                    }
                    dataPerEvent.push_back(nb_dataPts);

                    if (event->mTheta.mSamplerProposal == MHVariable::eFixe) {
                        evPts.Xmin =  event->mTheta.mRawTrace->at(0);
                        evPts.Xmax =  event->mTheta.mRawTrace->at(0);
                        evPts.Ymin = pt_Ymin;
                        evPts.Ymax = pt_Ymax;
                        evPts.color = event->mColor;
                        evPts.type = CurveRefPts::eRoundLine;;
                        evPts.comment = event->getQStringName();
                        // memo Data Points
                        eventsPts.append(evPts);
                        hpdPerEvent.push_back(1);

                    } else {

                        const QList<QPair<double, QPair<double, double> > > &intervals = event->mTheta.mRawHPDintervals;

                        for (const auto& h : intervals) {
                            evPts.Xmin =  h.second.first;
                            evPts.Xmax =  h.second.second;
                            evPts.Ymin = pt_Ymin;
                            evPts.Ymax = pt_Ymax;
                            evPts.color = event->mColor;
                            evPts.type = CurveRefPts::eLine;
                            evPts.comment = event->getQStringName();
                            // memo Data Points
                            eventsPts.append(evPts);
                        }

                        if (intervals.size() > 1) {
                            evPts.Xmin = intervals.first().second.first;
                            evPts.Xmax = intervals.last().second.second;
                            evPts.Ymin = pt_Ymin;
                            evPts.Ymax = pt_Ymax;
                            evPts.color = event->mColor;
                            evPts.type = CurveRefPts::eDotLine;
                            evPts.comment = event->getQStringName();
                            // memo Data Points
                            eventsPts.append(evPts);

                            hpdPerEvent.push_back((int)intervals.size() + 1);

                        } else if (intervals.size() == 1) {
                            hpdPerEvent.push_back(1);
                        }
                    }

                } else {

                    evPts.Xmin = static_cast<Bound*>(event.get())->mFixed;
                    evPts.Xmax = static_cast<Bound*>(event.get())->mFixed;
                    evPts.Ymin = pt_Ymin;
                    evPts.Ymax = pt_Ymax;
                    evPts.type = CurveRefPts::ePoint;
                    evPts.color = event->mColor;
                    evPts.comment = event->getQStringName();

                    dPts.Xmin = evPts.Xmin;//event->mTheta.mX; // always the same value
                    dPts.Xmax = evPts.Xmax;

                    dPts.Ymin = pt_Ymin;
                    dPts.Ymax = pt_Ymax;
                    dPts.color = event->mColor;
                    dPts.type = CurveRefPts::eRoundLine;
                    dPts.comment = event->getQStringName();

                    // memo Data Points
                    dataPts.append(dPts);
                    eventsPts.append(evPts);
                    hpdPerEvent.push_back(1);
                    dataPerEvent.push_back(1);
                }
            }
        }
        dynamic_cast<GraphViewCurve*>(mByCurvesGraphs[0])->setEventsPoints(eventsPts);
        dynamic_cast<GraphViewCurve*>(mByCurvesGraphs[0])->setDataPoints(dataPts);
    // End of ref point creation
    }
}

/*
void ResultsView::updateCurveEventsPointY()
{
    if (!isCurve())
        return;

    auto model = getModel_ptr();
    if (model == nullptr || !model->displayY()) {
        return;
    }


    // ----------------------------------------------------------------------
    // Show all events unless at least one is selected
    // ----------------------------------------------------------------------
    bool showAllEvents = ! model->hasSelectedEvents();


    double pt_Ymin(0.0), pt_Ymax(0.0);
    QList<CurveRefPts> eventsPts;
    QList<CurveRefPts> dataPts;
    // Stock the number of ref points per Event and Data
    std::vector<int> dataPerEvent;
    std::vector<int> hpdPerEvent;
    if (mMainVariable == GraphViewResults::eG) {
        //Same calcul within MultiCalibrationView::scatterPlot(const double thres)
        // Creation des points de ref
        int i = 0;
        int iDataPts = 0;
        int iEventPts = -1;
        for (const auto& event : model->mEvents) {
            if (event->mIsSelected || showAllEvents) {
                for (auto j = 0 ; j< hpdPerEvent[i]; j++) {
                    iEventPts++;
                    if ( model->mCurveSettings.mProcessType == CurveSettings::eProcess_3D ||
                         model->mCurveSettings.mProcessType == CurveSettings::eProcess_2D ) {
                        eventsPts[iEventPts].Ymin = event->mYDec;
                        eventsPts[iEventPts].Ymax = event->mYDec;

                    } else if ( model->mCurveSettings.mProcessType == CurveSettings::eProcess_Unknwon_Dec ) {
                        eventsPts[iEventPts].Ymin = event->mZField;
                        eventsPts[iEventPts].Ymax = event->mZField;

                    } else {
                        eventsPts[iEventPts].Ymin = event-> mYDec;
                        eventsPts[iEventPts].Ymax = event-> mYDec;
                    }

                }

                try {
                    for (auto j = 0 ; j< dataPerEvent[i]; j++) {

                        if ( model->mCurveSettings.mProcessType == CurveSettings::eProcess_3D ||
                            model->mCurveSettings.mProcessType == CurveSettings::eProcess_2D ) {
                            dataPts[iDataPts].Ymin = event->mYDec - 1.96*event->mS_Y;
                            dataPts[iDataPts].Ymax = event->mYDec + 1.96*event->mS_Y;

                        } else if ( model->mCurveSettings.mProcessType == CurveSettings::eProcess_Unknwon_Dec ) {
                            dataPts[iDataPts].Ymin = event->mZField - 1.96*event->mS_ZField;
                            dataPts[iDataPts].Ymax = event->mZField + 1.96*event->mS_ZField;

                        } else {
                            dataPts[iDataPts].Ymin = event-> mYDec - 1.96*(event->mS_XA95Depth/2.448) / cos(event->mXIncDepth * M_PI /180.);
                            dataPts[iDataPts].Ymax = event-> mYDec + 1.96*(event->mS_XA95Depth /2.448)/ cos(event->mXIncDepth * M_PI /180.);
                        }
                        iDataPts++;
                    }
                } catch (...) {
                    qDebug() << "[ResultView::updateCurveEventPointY] pb graphY";
                }

                ++i;
            }
        }
                // Set X = time

                if (event->mType == Event::eDefault) {
                    int nb_dataPts = 0;
                    for (const auto& date: event->mDates) {
                        // --- Calibration Date

                        const std::map<double, double> &calibMap = date.getRawCalibMap();
                        // hpd is calculate only on the study Period

                        // hpd results
                        QList<QPair<double, QPair<double, double> > > intervals ;
                        create_HPD_by_dichotomy(calibMap, intervals, date.mTi.mThresholdUsed);
                        // -- Post Distrib of Ti

                        if (intervals.size() > 1) {
                            for (const auto& h : std::as_const(intervals)) {
                                dPts.Xmin = h.second.first;
                                dPts.Xmax = h.second.second;
                                dPts.Ymin = pt_Ymin;
                                dPts.Ymax = pt_Ymax;
                                dPts.color = event->mColor;
                                dPts.type = CurveRefPts::eLine;
                                // memo Data Points
                                nb_dataPts++;
                                dataPts.append(dPts);
                            }
                            dPts.Xmin = intervals.first().second.first;
                            dPts.Xmax = intervals.last().second.second;
                            dPts.Ymin = pt_Ymin;
                            dPts.Ymax = pt_Ymax;
                            dPts.color = event->mColor;
                            dPts.type = CurveRefPts::eDotLineCross;
                            dPts.comment = event->getQStringName();
                            // memo Data Points
                            nb_dataPts++;
                            dataPts.append(dPts);


                        } else if (intervals.size() == 1) {

                            dPts.Xmin = intervals.first().second.first;
                            dPts.Xmax = intervals.last().second.second;
                            dPts.Ymin = pt_Ymin;
                            dPts.Ymax = pt_Ymax;
                            dPts.color = event->mColor;
                            dPts.type = CurveRefPts::eCross;
                            dPts.comment = event->getQStringName();
                            // memo Data Points
                            nb_dataPts++;
                            dataPts.append(dPts);
                        }

                    }
                    dataPerEvent.push_back(nb_dataPts);

                    if (event->mTheta.mSamplerProposal == MHVariable::eFixe) {
                        evPts.Xmin =  event->mTheta.mRawTrace->at(0);
                        evPts.Xmax =  event->mTheta.mRawTrace->at(0);
                        evPts.Ymin = pt_Ymin;
                        evPts.Ymax = pt_Ymax;
                        evPts.color = event->mColor;
                        evPts.type = CurveRefPts::eRoundLine;;
                        evPts.comment = event->getQStringName();
                        // memo Data Points
                        eventsPts.append(evPts);
                        hpdPerEvent.push_back(1);

                    } else {

                        const QList<QPair<double, QPair<double, double> > > &intervals = event->mTheta.mRawHPDintervals;

                        for (const auto& h : intervals) {
                            evPts.Xmin =  h.second.first;
                            evPts.Xmax =  h.second.second;
                            evPts.Ymin = pt_Ymin;
                            evPts.Ymax = pt_Ymax;
                            evPts.color = event->mColor;
                            evPts.type = CurveRefPts::eLine;
                            evPts.comment = event->getQStringName();
                            // memo Data Points
                            eventsPts.append(evPts);
                        }

                        if (intervals.size() > 1) {
                            evPts.Xmin = intervals.first().second.first;
                            evPts.Xmax = intervals.last().second.second;
                            evPts.Ymin = pt_Ymin;
                            evPts.Ymax = pt_Ymax;
                            evPts.color = event->mColor;
                            evPts.type = CurveRefPts::eDotLine;
                            evPts.comment = event->getQStringName();
                            // memo Data Points
                            eventsPts.append(evPts);

                            hpdPerEvent.push_back((int)intervals.size() + 1);

                        } else if (intervals.size() == 1) {
                            hpdPerEvent.push_back(1);
                        }
                    }

                } else {

                    evPts.Xmin = static_cast<Bound*>(event.get())->mFixed;
                    evPts.Xmax = static_cast<Bound*>(event.get())->mFixed;
                    evPts.Ymin = pt_Ymin;
                    evPts.Ymax = pt_Ymax;
                    evPts.type = CurveRefPts::ePoint;
                    evPts.color = event->mColor;
                    evPts.comment = event->getQStringName();

                    dPts.Xmin = evPts.Xmin;//event->mTheta.mX; // always the same value
                    dPts.Xmax = evPts.Xmax;

                    dPts.Ymin = pt_Ymin;
                    dPts.Ymax = pt_Ymax;
                    dPts.color = event->mColor;
                    dPts.type = CurveRefPts::eRoundLine;
                    dPts.comment = event->getQStringName();

                    // memo Data Points
                    dataPts.append(dPts);
                    eventsPts.append(evPts);
                    hpdPerEvent.push_back(1);
                    dataPerEvent.push_back(1);
                }
            }
        }
        dynamic_cast<GraphViewCurve*>(mByCurvesGraphs[1])->setEventsPoints(eventsPts);
        dynamic_cast<GraphViewCurve*>(mByCurvesGraphs[1])->setDataPoints(dataPts);
    // End of ref point creation
    }
}
void ResultsView::updateCurveEventsPointZ()
{
    if (!isCurve())
        return;

    auto model = getModel_ptr();
    if (model == nullptr) {
        return;
    }

    // ----------------------------------------------------------------------
    // Show all events unless at least one is selected
    // ----------------------------------------------------------------------
    bool showAllEvents = ! model->hasSelectedEvents();


    double pt_Ymin(0.0), pt_Ymax(0.0);
    QList<CurveRefPts> eventsPts;
    QList<CurveRefPts> dataPts;
    // Stock the number of ref points per Event and Data
    std::vector<int> dataPerEvent;
    std::vector<int> hpdPerEvent;
    if (mMainVariable == GraphViewResults::eG) {
        //Same calcul within MultiCalibrationView::scatterPlot(const double thres)
        // Creation des points de ref
        for (const auto& event : model->mEvents) {
            if (event->mIsSelected || showAllEvents) {
                CurveRefPts evPts;
                CurveRefPts dPts;
                double verr;
                // Set Y for graph X

                switch (model->mCurveSettings.mProcessType) {
                case CurveSettings::eProcess_Inclination :
                    verr = event->mS_XA95Depth / 2.448;
                    pt_Ymin = event->mXIncDepth - 1.96*verr;
                    pt_Ymax = event->mXIncDepth + 1.96*verr;
                    break;
                case CurveSettings::eProcess_Declination :
                    verr = (event->mS_XA95Depth/2.448) / cos(event->mXIncDepth * M_PI /180.);
                    pt_Ymin = event->mYDec - 1.96*verr;
                    pt_Ymax = event->mYDec + 1.96*verr;
                    break;
                case CurveSettings::eProcess_Field :
                    pt_Ymin = event->mZField - 1.96*event->mS_ZField;
                    pt_Ymax = event->mZField + 1.96*event->mS_ZField;
                    break;
                case CurveSettings::eProcess_Depth :
                    pt_Ymin = event->mXIncDepth - 1.96*event->mS_XA95Depth;
                    pt_Ymax = event->mXIncDepth + 1.96*event->mS_XA95Depth;
                    break;

                case CurveSettings::eProcess_3D:
                case CurveSettings::eProcess_2D:
                case CurveSettings::eProcess_Univariate :
                case CurveSettings::eProcess_Unknwon_Dec:
                    pt_Ymin = event->mXIncDepth - 1.96*event->mS_XA95Depth;
                    pt_Ymax = event->mXIncDepth + 1.96*event->mS_XA95Depth;
                    break;

                case CurveSettings::eProcess_Spherical:
                case CurveSettings::eProcess_Vector:
                    verr = event->mS_XA95Depth / 2.448;
                    pt_Ymin = event->mXIncDepth - 1.96*verr;
                    pt_Ymax = event->mXIncDepth + 1.96*verr;
                    break;

                case CurveSettings::eProcess_None:
                default:
                    break;

                }
                evPts.color = event->mColor;

                // Set X = time

                if (event->mType == Event::eDefault) {
                    int nb_dataPts = 0;
                    for (const auto& date: event->mDates) {
                        // --- Calibration Date

                        const std::map<double, double> &calibMap = date.getRawCalibMap();
                        // hpd is calculate only on the study Period

                        // hpd results
                        QList<QPair<double, QPair<double, double> > > intervals ;
                        create_HPD_by_dichotomy(calibMap, intervals, date.mTi.mThresholdUsed);
                        // -- Post Distrib of Ti

                        if (intervals.size() > 1) {
                            for (const auto& h : std::as_const(intervals)) {
                                dPts.Xmin = h.second.first;
                                dPts.Xmax = h.second.second;
                                dPts.Ymin = pt_Ymin;
                                dPts.Ymax = pt_Ymax;
                                dPts.color = event->mColor;
                                dPts.type = CurveRefPts::eLine;
                                // memo Data Points
                                nb_dataPts++;
                                dataPts.append(dPts);
                            }
                            dPts.Xmin = intervals.first().second.first;
                            dPts.Xmax = intervals.last().second.second;
                            dPts.Ymin = pt_Ymin;
                            dPts.Ymax = pt_Ymax;
                            dPts.color = event->mColor;
                            dPts.type = CurveRefPts::eDotLineCross;
                            dPts.comment = event->getQStringName();
                            // memo Data Points
                            nb_dataPts++;
                            dataPts.append(dPts);


                        } else if (intervals.size() == 1) {

                            dPts.Xmin = intervals.first().second.first;
                            dPts.Xmax = intervals.last().second.second;
                            dPts.Ymin = pt_Ymin;
                            dPts.Ymax = pt_Ymax;
                            dPts.color = event->mColor;
                            dPts.type = CurveRefPts::eCross;
                            dPts.comment = event->getQStringName();
                            // memo Data Points
                            nb_dataPts++;
                            dataPts.append(dPts);
                        }

                    }
                    dataPerEvent.push_back(nb_dataPts);

                    if (event->mTheta.mSamplerProposal == MHVariable::eFixe) {
                        evPts.Xmin =  event->mTheta.mRawTrace->at(0);
                        evPts.Xmax =  event->mTheta.mRawTrace->at(0);
                        evPts.Ymin = pt_Ymin;
                        evPts.Ymax = pt_Ymax;
                        evPts.color = event->mColor;
                        evPts.type = CurveRefPts::eRoundLine;;
                        evPts.comment = event->getQStringName();
                        // memo Data Points
                        eventsPts.append(evPts);
                        hpdPerEvent.push_back(1);

                    } else {

                        const QList<QPair<double, QPair<double, double> > > &intervals = event->mTheta.mRawHPDintervals;

                        for (const auto& h : intervals) {
                            evPts.Xmin =  h.second.first;
                            evPts.Xmax =  h.second.second;
                            evPts.Ymin = pt_Ymin;
                            evPts.Ymax = pt_Ymax;
                            evPts.color = event->mColor;
                            evPts.type = CurveRefPts::eLine;
                            evPts.comment = event->getQStringName();
                            // memo Data Points
                            eventsPts.append(evPts);
                        }

                        if (intervals.size() > 1) {
                            evPts.Xmin = intervals.first().second.first;
                            evPts.Xmax = intervals.last().second.second;
                            evPts.Ymin = pt_Ymin;
                            evPts.Ymax = pt_Ymax;
                            evPts.color = event->mColor;
                            evPts.type = CurveRefPts::eDotLine;
                            evPts.comment = event->getQStringName();
                            // memo Data Points
                            eventsPts.append(evPts);

                            hpdPerEvent.push_back((int)intervals.size() + 1);

                        } else if (intervals.size() == 1) {
                            hpdPerEvent.push_back(1);
                        }
                    }

                } else {

                    evPts.Xmin = static_cast<Bound*>(event.get())->mFixed;
                    evPts.Xmax = static_cast<Bound*>(event.get())->mFixed;
                    evPts.Ymin = pt_Ymin;
                    evPts.Ymax = pt_Ymax;
                    evPts.type = CurveRefPts::ePoint;
                    evPts.color = event->mColor;
                    evPts.comment = event->getQStringName();

                    dPts.Xmin = evPts.Xmin;//event->mTheta.mX; // always the same value
                    dPts.Xmax = evPts.Xmax;

                    dPts.Ymin = pt_Ymin;
                    dPts.Ymax = pt_Ymax;
                    dPts.color = event->mColor;
                    dPts.type = CurveRefPts::eRoundLine;
                    dPts.comment = event->getQStringName();

                    // memo Data Points
                    dataPts.append(dPts);
                    eventsPts.append(evPts);
                    hpdPerEvent.push_back(1);
                    dataPerEvent.push_back(1);
                }
            }
        }
        dynamic_cast<GraphViewCurve*>(mByCurvesGraphs[2])->setEventsPoints(eventsPts);
        dynamic_cast<GraphViewCurve*>(mByCurvesGraphs[2])->setDataPoints(dataPts);
    // End of ref point creation
    }
}
*/

void ResultsView::deleteAllGraphsInList(QList<GraphViewResults*> &list)
{
    //Pas besoin de déconnecter les signaux manuellement avant de détruire un QWidget

    qDeleteAll(list);
    list.clear();
    list.squeeze(); // équivalent Qt à shrink_to_fit()
}

QList<GraphViewResults*> ResultsView::allGraphs()
{
    QList<GraphViewResults*> graphs;
    graphs.append(mByEventsGraphs);
    graphs.append(mByPhasesGraphs);
    graphs.append(mByCurvesGraphs);
    return graphs;
}

bool ResultsView::hasSelectedGraphs()
{
    return (currentGraphs(true).size() > 0);
}

QList<GraphViewResults*> ResultsView::currentGraphs(bool onlySelected)
{
    QList<GraphViewResults*> selectedGraphs;

    int index = mGraphListTab->currentIndex();
    switch (index) {
    case 0:
        selectedGraphs = mByEventsGraphs;
        break;
    case 1:
        selectedGraphs = mByPhasesGraphs;
        break;
    case 2:
        selectedGraphs = mByCurvesGraphs;
        break;
    default:
        selectedGraphs = QList<GraphViewResults*>();
        break;
    }

    if (onlySelected && !selectedGraphs.isEmpty()) {
        QList<GraphViewResults*> selectedGraphsList;
        for (GraphViewResults* graph : selectedGraphs) {
            if (graph->isSelected()) {
                selectedGraphsList.append(graph);
            }
        }
        return selectedGraphsList;
    }

    return selectedGraphs;
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
    const auto str_tip = getModel_ptr()->getCurvesName();

    int i = 0;
    for (GraphViewResults*& graphView : listGraphs) {
        graphView->generateCurves(GraphViewResults::graph_t(mCurrentTypeGraph), mCurrentVariableList);
        if (mCurrentVariableList.contains(GraphViewResults::eG))
            graphView->setTipYLab(str_tip.at(i++));
    }

    updateGraphsMinMax();
    updateScales(); // contient updateCurvesToShow pour les courbes

}

void ResultsView::updateGraphsMinMax()
{
    auto model = getModel_ptr();
    QList<GraphViewResults*> listGraphs = currentGraphs(false);
    if (mCurrentTypeGraph == GraphViewResults::ePostDistrib) {

        if (mMainVariable == GraphViewResults::eDuration
#ifdef S02_BAYESIAN
            || mMainVariable == GraphViewResults::eS02
#endif

#ifdef KOMLAN
            || mMainVariable == GraphViewResults::eS02Vg
#endif
            ) {
            mResultMinT = 0.;
            mResultMaxT = getGraphsMax(listGraphs, "Post Distrib", 0.0);

        } else if (mMainVariable == GraphViewResults::eSigma) {
            mResultMinT = 0;
            mResultMaxT = getGraphsMax(listGraphs, "Post Distrib", 0.0) / 3.;

        } else if (mMainVariable == GraphViewResults::eVg) {
            mResultMinT = 0;
            mResultMaxT = getGraphsMax(listGraphs, "Post Distrib", 0.0) / 3.;

        } else if (mMainVariable == GraphViewResults::eLambda) {
            mResultMinT = getGraphsMin(listGraphs, "Lambda", -20.);
            mResultMaxT = getGraphsMax(listGraphs, "Lambda", 10.);

        } else {
            mResultMinT = model->mSettings.getTminFormated();
            mResultMaxT = model->mSettings.getTmaxFormated();
        }

    } else if ((mCurrentTypeGraph == GraphViewResults::eTrace) || (mCurrentTypeGraph == GraphViewResults::eAccept)) {
            for (qsizetype i = 0; i<mChainRadios.size(); ++i) {
                if (mChainRadios.at(i)->isChecked()) {
                    const ChainSpecs& chain = model->mChains.at(i);
                    mResultMinT = 0;
                    const int adaptSize = chain.mBatchIndex * chain.mIterPerBatch;
                    const int runSize = chain.mRealyAccepted;
                    mResultMaxT = 1 + chain.mIterPerBurn + adaptSize + runSize;
                    break;
                }
            }

    } else if (mCurrentTypeGraph == GraphViewResults::eCorrel) {
        mResultMinT = 0.;
        mResultMaxT = 40;

    }
}

double ResultsView::getGraphsMax(const QList<GraphViewResults*> &graphs, const QString &title, double maxFloor)
{
    for (const auto& graphWrapper : graphs) {
        const QList<GraphCurve> &curves = graphWrapper->getGraph()->getCurves();
        for (const auto& curve : curves) {
            if (!curve.mData.isEmpty() && curve.mName.contains(title)) {
                maxFloor = std::max(maxFloor, curve.mData.lastKey());
            }
        }
    }

    return maxFloor;
}

double ResultsView::getGraphsMin(const QList<GraphViewResults*> &graphs, const QString &title, double minCeil)
{
    for (const auto& graphWrapper : graphs) {
        const QList<GraphCurve> &curves = graphWrapper->getGraph()->getCurves();
        for (const auto& curve : curves) {
            if (!curve.mData.isEmpty() && curve.mName.contains(title) && (curve.mVisible == true)) {
                minCeil = std::min(minCeil, curve.mData.firstKey());
            }
        }
    }

    return minCeil;
}

/**
 *  @brief Decide which curve graphs must be show, based on currently selected options.
 *  @brief This function does NOT remove or create any curve in graphs! It only checks if existing curves should be visible or not.
 */
void ResultsView::updateCurvesToShow()
{
    auto model = getModel_ptr();
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
    QList<GraphViewResults::variable_t> showVariableList;
    // --------------------------------------------------------
    //  Options for "Curves"
    // --------------------------------------------------------
    if ((mGraphListTab->currentName() == tr("Curves"))
            && !mLambdaRadio->isChecked()
#ifdef KOMLAN
            && !mS02VgRadio->isChecked()
#endif
            ) {

        if (mCurveGRadio->isChecked()) {
            showVariableList.append(GraphViewResults::eG);

            if (mCurveErrorCheck->isChecked())
                showVariableList.append(GraphViewResults::eGGauss);

            if (mCurveHpdCheck->isChecked())
                showVariableList.append(GraphViewResults::eGHpd);

            if (mCurveMapCheck->isChecked())
                showVariableList.append(GraphViewResults::eMap);

            if (mCurveEventsPointsCheck->isChecked())
                showVariableList.append(GraphViewResults::eGEventsPts);

            if (mCurveDataPointsCheck->isChecked())
                showVariableList.append(GraphViewResults::eGDatesPts);
        }
        if (mCurveGPRadio->isChecked()) {
            showVariableList.append(GraphViewResults::eGP);

            if (mCurveGPGaussCheck->isChecked())
                showVariableList.append(GraphViewResults::eGPGauss);

            if (mCurveGPHpdCheck->isChecked())
                showVariableList.append(GraphViewResults::eGPHpd);

            if (mCurveGPMapCheck->isChecked())
                showVariableList.append(GraphViewResults::eGPMap);


        }

       if (mCurveGSRadio->isChecked())
            showVariableList.append(GraphViewResults::eGS);

       const bool showStat = mCurveStatCheck->isChecked();

        // --------------------------------------------------------
        //  Update Graphs with selected options
        // --------------------------------------------------------
        for (GraphViewResults*& graph : listGraphs) {

            GraphViewCurve* graphCurve = static_cast<GraphViewCurve*>(graph);
            graphCurve->setShowNumericalResults(showStat);

            QString graphName = model->getCurvesLongName().at(0);
            const QString varRateText = tr("Variation Rate");
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

            if (model->displayY() ) {
                graphName = model->getCurvesLongName().at(1);
                if (mCurveGPRadio->isChecked())
                    graphName = graphName + " " + varRateText;

                else if (mCurveGSRadio->isChecked())
                    graphName = graphName + " " + accelarationText;

                if (graph->title() == graphName) {
                    Scale scaleY;
                    scaleY.findOptimalMark(mResultCurrentMinY, mResultCurrentMaxY, 10);
                    graphCurve->updateCurvesToShowForG(showAllChains, showChainList, showVariableList, scaleY);
                }

                if (model->displayZ()) {
                    graphName = model->getCurvesLongName().at(2);
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

    } else if ((mGraphListTab->currentName() == tr("Curves")) && mLambdaRadio->isChecked()) {
        if (mCredibilityCheck->isChecked())
            showVariableList.append(GraphViewResults::eCredibility);
        showVariableList.append(GraphViewResults::eLambda);

#ifdef KOMLAN
    }  else if ((mGraphListTab->currentName() == tr("Curves")) && mS02VgRadio->isChecked()) {
            if (mCredibilityCheck->isChecked())
                showVariableList.append(GraphViewResults::eCredibility);
            showVariableList.append(GraphViewResults::eS02Vg);

#endif

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
#ifdef S02_BAYESIAN
        else if (mEventVGRadio->isChecked()) {
            if (mCredibilityCheck->isChecked())
                showVariableList.append(GraphViewResults::eCredibility);
            showVariableList.append(GraphViewResults::eS02);
        }
#endif

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
    const bool showStat = mEventsStatCheck->isChecked();
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
 *  - Set Ruler and graphs range and mZoomsT
 *  - Update mXMinEdit, mXMaxEdit, mXSlider, mTimeSpin, mMajorScaleEdit, mMinorScaleEdit
 *  - Set slider and  zoomEdit with mZoomsH
 */
void ResultsView::updateScales()
{
    auto model = getModel_ptr();
    if ( model == nullptr) {
        return;
    }

    const StudyPeriodSettings s = model->mSettings;

    // ------------------------------------------------------------------
    //  Define mResultCurrentMinT and mResultCurrentMaxT
    //  + Restore last zoom values if any
    // ------------------------------------------------------------------

    // The key of the saved zooms map is as long as that :
    QPair<GraphViewResults::variable_t, GraphViewResults::graph_t> key(mMainVariable, mCurrentTypeGraph);

    // Anyway, let's check if we have a saved zoom value for this key :
    if (mZoomsT.find(key) != mZoomsT.end()) {
        // Get the saved (unformatted) values
        const double tMin = mZoomsT.value(key).first;
        const double tMax = mZoomsT.value(key).second;

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
                    const ChainSpecs& chain = model->mChains.at(i);
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
    if (mScalesT.find(key) != mScalesT.end()) {
        mMajorScale = mScalesT.value(key).first;
        mMinorCountScale = mScalesT.value(key).second;

    } else {
        // For correlation graphs, ticks intervals are not an available option
        if (mCurrentTypeGraph == GraphViewResults::eCorrel) {
            mMajorScale = 10.;
            mMinorCountScale = 10;
        }
        // All other cases (default behavior)
        else {
            Scale t_scale;

            t_scale.findOptimal(mResultCurrentMinT, mResultCurrentMaxT, 10);
            mMajorScale = t_scale.mark;
            mMinorCountScale = t_scale.tip;
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
        mTimeSlider->setRange(-100, 100);

        // The Ruler range is much wider based on the minimal zoom
        const double tCenter = (mResultCurrentMinT + mResultCurrentMaxT) / 2.;
        const double tSpan = mResultCurrentMaxT - mResultCurrentMinT;
        const double tRangeMin = tCenter - ((tSpan/2.) / sliderToZoom(mTimeSlider->minimum()));
        const double tRangeMax = tCenter + ((tSpan/2.) / sliderToZoom(mTimeSlider->minimum()));

        mRuler->setRange(tRangeMin, tRangeMax);
        mRuler->setFormatFunctX(nullptr);

    } else if (mCurrentTypeGraph == GraphViewResults::ePostDistrib &&
               ( mMainVariable == GraphViewResults::eSigma
#ifdef S02_BAYESIAN
                || mMainVariable == GraphViewResults::eS02
#endif

#ifdef KOMLAN
                || mMainVariable == GraphViewResults::eS02Vg
#endif
                || mMainVariable == GraphViewResults::eVg
                || mMainVariable == GraphViewResults::eDuration) ) {

                // The X zoom uses a log scale on the spin box and can be controlled by the linear slider
                mTimeSlider->setRange(-100, 100);

                // The Ruler range is much wider based on the minimal zoom
                const double tRangeMax = mResultMaxT / sliderToZoom(mTimeSlider->minimum());

                mRuler->setRange(0, tRangeMax);
                mRuler->setFormatFunctX(nullptr);



    } else if (mCurrentTypeGraph == GraphViewResults::ePostDistrib &&
               mMainVariable == GraphViewResults::eLambda ) {

                // The X zoom uses a log scale on the spin box and can be controlled by the linear slider
                mTimeSlider->setRange(-100, 100);

                // The Ruler range is much wider based on the minimal zoom
                const double tCenter = (mResultCurrentMinT + mResultCurrentMaxT) / 2.;
                const double tSpan = mResultCurrentMaxT - mResultCurrentMinT;
                const double tRangeMin = tCenter - ((tSpan/2.) / sliderToZoom(mTimeSlider->minimum()));
                const double tRangeMax = tCenter + ((tSpan/2.) / sliderToZoom(mTimeSlider->minimum()));

                mRuler->setRange(tRangeMin, tRangeMax);
                mRuler->setFormatFunctX(nullptr);

    } else if ( mCurrentTypeGraph == GraphViewResults::eTrace ||
                mCurrentTypeGraph == GraphViewResults::eAccept) {

        int idSelect = 0;
        for (const auto& chRadio : std::as_const(mChainRadios)) {
            if (chRadio->isChecked())
                break;
            ++idSelect;
        }

        const ChainSpecs& chain = model->mChains.at(idSelect);
        const int adaptSize = chain.mBatchIndex * chain.mIterPerBatch;
        const int runSize = chain.mRealyAccepted;
        mResultMaxT = 1 +  chain.mIterPerBurn + adaptSize + runSize;
        // The min is always 0
        mResultMinT = 0.;

        mRuler->addArea(0.0, 1+ chain.mIterPerBurn, QColor(235, 115, 100));
        mRuler->addArea(1 + chain.mIterPerBurn, 1 + chain.mIterPerBurn + adaptSize, QColor(250, 180, 90));
        mRuler->addArea(1 + chain.mIterPerBurn + adaptSize, mResultMaxT, QColor(130, 205, 110));
        // The Ruler range is set exactly to the min and max (impossible to scroll outside)
        mRuler->setRange(mResultMinT, mResultMaxT);
        mRuler->setFormatFunctX(nullptr);


        // The zoom slider and spin are linear.
        // The number of zoom levels depends on the number of iterations (by a factor 100)
        // e.g. 400 iterations => 4 levels
        const int zoomLevels = (int) mResultMaxT / 100;
        mTimeSlider->setRange(1, zoomLevels);
        //mTimeSpin->setRange(1, zoomLevels);
        //mTimeSpin->setSingleStep(1.);
        //mTimeSpin->setDecimals(0);

        // find new minY and maxY


    } else if ( mCurrentTypeGraph == GraphViewResults::eCorrel) {
        // The x axis represents h, always in [0, 40]
        mResultMinT = 0.;
        mResultMaxT = 40.;

        // The zoom slider and spin are linear.
        // Always 5 zoom levels
        mTimeSlider->setRange(1, 5);
        //mTimeSpin->setRange(1, 5);
        //mTimeSpin->setSingleStep(1.);
        //mTimeSpin->setDecimals(0);

        mRuler->setRange(mResultMinT, mResultMaxT);
        mRuler->setFormatFunctX(nullptr);
    }


    // ------------------------------------------------------------------
    //  Apply to Ruler
    // ------------------------------------------------------------------
    mRuler->setCurrent(mResultCurrentMinT, mResultCurrentMaxT);
    mRuler->setScaleDivision(mMajorScale, mMinorCountScale);


    // -------------------------------------------------------
    //  Set options UI components values
    // -------------------------------------------------------
    setTimeRange();
    setTimeSlider(zoomToSlider(mResultZoomT));
    setTimeEdit(mResultZoomT);
    setTimeScale();

    // -------------------------------------------------------
    // Graphic Option
    // -------------------------------------------------------
    //double origin_height;
    if (mGraphListTab->currentIndex() == 2 )
        mHeightForVisibleAxis = 20 * AppSettings::heigthUnit() / mByCurvesGraphs.size();
    else
        mHeightForVisibleAxis = 4 * AppSettings::heigthUnit() ;
    //mGraphHeight = GraphViewResults::mHeightForVisibleAxis;

    int zoom = 100;
    if (mZoomsH.find(key) != mZoomsH.end()) {
        zoom = mZoomsH.value(key);
    }
    mZoomEdit->blockSignals(true);
    mZoomEdit->setText(QLocale().toString(zoom));
    mZoomEdit->blockSignals(false);

    mZoomSlider->blockSignals(true);
    mZoomSlider->setValue(zoom);
    mZoomSlider->blockSignals(false);

    const double min = 2.0 * AppSettings::heigthUnit();
    const double origin = mHeightForVisibleAxis;

    const double prop = zoom / 100.0;
    mGraphHeight = min + prop * (origin - min);

    // -------------------------------------------------------
    //  Apply to all graphs
    // -------------------------------------------------------
    QList<GraphViewResults*> graphs = currentGraphs(false);
    for (GraphViewResults*& graph : graphs) {
        graph->setHeightForVisibleAxis(mHeightForVisibleAxis);
        graph->setView(mRuler->mMin, mRuler->mMax, mResultCurrentMinT, mResultCurrentMaxT, mMajorScale, mMinorCountScale);
    }

    // -------------------------------------------------------
    // X option
    // -------------------------------------------------------
    if (model->displayX()) {
        if (mZoomsX.find(key) != mZoomsX.end()) {
            const double XMin = mZoomsX.value(key).first;
            const double XMax = mZoomsX.value(key).second;

            mResultCurrentMinX = XMin;
            mResultCurrentMaxX = XMax;
            setXRange();

        } else {
            findOptimalX();
        }
    }
    // -------------------------------------------------------
    // Y option
    // -------------------------------------------------------
    if (model->displayY()) {
        if (mZoomsY.find(key) != mZoomsY.end()) {
            const double YMin = mZoomsY.value(key).first;
            const double YMax = mZoomsY.value(key).second;

            mResultCurrentMinY = YMin;
            mResultCurrentMaxY = YMax;
            setYRange();

        } else {
            findOptimalY();
        }
        // -------------------------------------------------------
        // Z option
        // -------------------------------------------------------
        if (model->displayZ()) {
            if (mZoomsZ.find(key) != mZoomsZ.end()) {
                const double ZMin = mZoomsZ.value(key).first;
                const double ZMax = mZoomsZ.value(key).second;

                mResultCurrentMinZ = ZMin;
                mResultCurrentMaxZ = ZMax;
                setZRange();

            } else {
                findOptimalZ();
            }
        }
    }

    updateCurvesToShow();

}

void ResultsView::createOptionsWidget()
{

    unsigned optionWidgetHeight = 0;
    // -------------------------------------------------------------------------------------
    //  Update graph list tab on the top left
    // -------------------------------------------------------------------------------------

    mGraphListTab->setTabVisible(1, mHasPhases); // Phases
    //mGraphListTab->setTabVisible(2, isCurve()); // Curve


    delete mOptionsLayout;
    mOptionsLayout = new QVBoxLayout();
    mOptionsLayout->setContentsMargins(mMargin, mMargin, 0, 0);
    mOptionsLayout->addWidget(mGraphListTab);
    // If the current tab is not currently visible :
    // - Show the "Phases" tab (1) which is a good default choice if the model has phases.
    // - Show the "Events" tab (0) which is a good default choice if the model doesn't have phases.

    //if (mHasPhases && mGraphListTab->currentIndex() >= 2 && !isCurve()) {
        mGraphListTab->setTab(1, false);



    optionWidgetHeight += mGraphListTab->height();
    // --------


    // Ajustement final de la hauteur
    mOptionsWidget->setFixedHeight(optionWidgetHeight + 20);



    // -------------------------------------------------------------------------------------
    //  Update controls depending on current graph list
    // -------------------------------------------------------------------------------------

        mOptionsLayout->addWidget(mEventsGroup);

        mGraphTypeTabs->setTabVisible(1, true); // History Plot
        mGraphTypeTabs->setTabVisible(2, true); // Acceptance Rate
        mGraphTypeTabs->setTabVisible(3, true); // Autocorrelation

        mEventsGroup->show();
        mPhasesGroup->setVisible(false);
        mCurvesGroup->setVisible(false);
        const qreal h = mEventThetaRadio->height() * 1.5;

        mEventVGRadio->setVisible(false);
        //--- change layout


        QVBoxLayout* eventGroupLayout = new QVBoxLayout();
        eventGroupLayout->setContentsMargins(10, 10, 10, 10);
        //eventGroupLayout->setSpacing(15);
        eventGroupLayout->addWidget(mEventThetaRadio);
        eventGroupLayout->addWidget(mDataSigmaRadio);
#ifdef S02_BAYESIAN
        eventGroupLayout->addWidget(mS02Radio);
        qreal totalH =  4*h;
 #ifdef KOMLAN
        eventGroupLayout->addWidget(mS02VgRadio);
        totalH =  5*h;
 #endif
#else
        qreal totalH =  3*h;
#endif

        mEventVGRadio->hide();


        eventGroupLayout->addWidget(mEventsDatesUnfoldCheck);


        mDataCalibCheck->hide();
        mWiggleCheck->hide();

        eventGroupLayout->addWidget(mEventsStatCheck);
        totalH += h;

        delete mEventsGroup->layout() ;
        mEventsGroup->setLayout(eventGroupLayout);

        //-- end new layout

        mEventsGroup->setFixedHeight(totalH);

        optionWidgetHeight += mEventsGroup->height();



    // ------------------------------------
    //  Display / Distrib. Option
    // ------------------------------------
    optionWidgetHeight += mDisplayDistribTab->height();

    mOptionsLayout->addWidget(mDisplayDistribTab);

    qreal widHeigth = 0;
    const  qreal internSpan = 10;
    if (true) {//mDisplayDistribTab->currentName() == tr("Display") ) {
        mDisplayGroup->show();
        mDistribGroup->hide();
        mOptionsLayout->addWidget(mDisplayGroup);

        // ------------------------------------
        //  Display Options
        // ------------------------------------

        const qreal h = mDisplayStudyBut->height();

        widHeigth = 11*h + 13*internSpan;
        // 11*h = spanOptionTitle + studyPeriodButton + span + slider + majorInterval + minorCount
        //       + GraphicOptionsTitle + ZoomSlider + Font + Thickness + Opacity



        mDisplayStudyBut->setText(xScaleRepresentsTime() ? tr("Study Period Display") : tr("Fit Display"));
        mDisplayStudyBut->setVisible(true);
        widHeigth += mDisplayStudyBut->height() + internSpan;


        mDisplayGroup-> setFixedHeight(widHeigth);


        optionWidgetHeight += widHeigth;

    }
    optionWidgetHeight += 35;//40; // ???

    // -------------------------------------------------------------------------------------
    //  Page / Save
    // -------------------------------------------------------------------------------------
    mOptionsLayout->addWidget(mPageSaveTab);
    optionWidgetHeight += mPageSaveTab->height();

    if (true) {//mPageSaveTab->currentName() == tr("Page") ) {

        // -------------------------------------------------------------------------------------
        //  - Update the total number of graphs for all pages
        //  - Check if the current page is still lower than the number of pages
        //  - Update the pagination display
        //  => All this must be done BEFORE calling createGraphs, which uses theses params to build the graphs
        // -------------------------------------------------------------------------------------
        //updateTotalGraphs();
        mMaximunNumberOfVisibleGraph = 0;

        const int numPages = ceil((double)mMaximunNumberOfVisibleGraph / (double)mGraphsPerPage);
        if (mCurrentPage >= numPages) {
            mCurrentPage = 0;
        }

        mPageEdit->setText(QLocale().toString(mCurrentPage + 1) + "/" + QLocale().toString(numPages));

        mPageWidget->setVisible(true);
        mSaveSelectWidget->hide();
        mSaveAllWidget->hide();

        mOptionsLayout->addWidget(mPageWidget);
        optionWidgetHeight += mPageWidget->height();

    }

    mOptionsLayout->addStretch();
    mOptionsWidget->setLayout(mOptionsLayout);

    mOptionsWidget->setGeometry(0, 0, mOptionsW - mMargin, optionWidgetHeight);
}



// --- Utils ------------------------------------------------------
inline void replaceLayout(QWidget* widget, QLayout* newLayout) {
    if (auto old = widget->layout()) delete old;
    widget->setLayout(newLayout);
}


inline void addTotalHeight(qreal &totalH, QWidget* w, qreal factor = 1.0)
{
    totalH += w->height() * factor + 2;
}

void ResultsView::updateEventsOptions(qreal& optionWidgetHeight, bool isPostDistrib)
{
    mOptionsLayout->addWidget(mEventsGroup);
    mGraphTypeTabs->setTabVisible(1, true);
    mGraphTypeTabs->setTabVisible(2, true);
    mGraphTypeTabs->setTabVisible(3, true);

    mEventsGroup->show();
    mPhasesGroup->hide();
    mCurvesGroup->hide();

    qreal totalH = 0;
    QVBoxLayout* eventLayout = new QVBoxLayout();
    eventLayout->setContentsMargins(10, 10, 10, 10);
    eventLayout->setSpacing(7);

    auto add = [&](QWidget* w) {
        w->show();
        eventLayout->addWidget(w);
        addTotalHeight(totalH, w);
    };


    add(mEventThetaRadio);
    add(mDataSigmaRadio);

#ifdef S02_BAYESIAN
    add(mS02Radio);
#endif

    if (isCurve()) {
        add(mEventVGRadio);

    } else {
        mEventVGRadio->hide();
    }
    add(mEventsDatesUnfoldCheck);

    if (isPostDistrib && mEventThetaRadio->isChecked() && mEventsDatesUnfoldCheck->isChecked()) {
        mDataCalibCheck->show();
        mWiggleCheck->show();

        QVBoxLayout* unfoldLayout = new QVBoxLayout();
        unfoldLayout->setContentsMargins(15, 0, 0, 0);
        unfoldLayout->setSpacing(10);
        unfoldLayout->addWidget(mDataCalibCheck, Qt::AlignLeft);
        unfoldLayout->addWidget(mWiggleCheck, Qt::AlignLeft);
        eventLayout->addLayout(unfoldLayout);

        addTotalHeight(totalH, mDataCalibCheck);
        addTotalHeight(totalH, mWiggleCheck);
        totalH += unfoldLayout->spacing() * 2;

    } else {
        mDataCalibCheck->hide();
        mWiggleCheck->hide();
    }

    add(mEventsStatCheck);

    totalH += eventLayout->contentsMargins().top() + eventLayout->contentsMargins().bottom();
    totalH += eventLayout->spacing() * 4;
    replaceLayout(mEventsGroup, eventLayout);
    mEventsGroup->setFixedHeight(totalH);
    optionWidgetHeight += totalH + 10;
}

void ResultsView::updatePhasesOptions(qreal &optionWidgetHeight)
{
    mOptionsLayout->addWidget(mPhasesGroup);
    const bool show_histo = mBeginEndRadio->isChecked() || mDurationRadio->isChecked();
    mGraphTypeTabs->setTabVisible(1, show_histo);

    const bool show_accept = mBeginEndRadio->isChecked();
    mGraphTypeTabs->setTabVisible(2, show_accept);

    const bool show_correl = mBeginEndRadio->isChecked();
    mGraphTypeTabs->setTabVisible(3, show_correl);

    mEventsGroup->setVisible(false);
    mPhasesGroup->setVisible(true);
    mCurvesGroup->setVisible(false);

    qreal totalH = 0;
    QVBoxLayout* phaseLayout = new QVBoxLayout();
    phaseLayout->setContentsMargins(10, 10, 10, 10);
    phaseLayout->setSpacing(7);
    auto add = [&](QWidget* w) {
        w->show();
        phaseLayout->addWidget(w);
        addTotalHeight(totalH, w);
    };

    add(mBeginEndRadio);
    add(mTempoRadio);
    if (mTempoRadio->isChecked()||mActivityRadio->isChecked() ) {
        mErrCheck->show();

    } else {
        mErrCheck->hide();
    }

    if (mTempoRadio->isChecked()) {

        QHBoxLayout* unfoldLayout = new QHBoxLayout();
        unfoldLayout->setContentsMargins(15, 0, 0, 0);
        unfoldLayout->setSpacing(10);
        unfoldLayout->addWidget(mErrCheck, Qt::AlignLeft);
        phaseLayout->addLayout(unfoldLayout);

        addTotalHeight(totalH, mErrCheck);
        totalH += unfoldLayout->spacing() * 1;
    }

    add(mActivityRadio);
    if (mActivityRadio->isChecked()) {
        mActivityUnifCheck->show();

        QVBoxLayout* unfoldLayout = new QVBoxLayout();
        unfoldLayout->setContentsMargins(15, 0, 0, 0);
        unfoldLayout->setSpacing(10);
        unfoldLayout->addWidget(mErrCheck, Qt::AlignLeft);
        unfoldLayout->addWidget(mActivityUnifCheck, Qt::AlignLeft);

        addTotalHeight(totalH, mErrCheck);
        addTotalHeight(totalH, mActivityUnifCheck);
        totalH += unfoldLayout->spacing() * 2;
        phaseLayout->addLayout(unfoldLayout);

    } else {
        mActivityUnifCheck->hide();
    }

    add(mDurationRadio);
    add(mPhasesEventsUnfoldCheck);
    if (mPhasesEventsUnfoldCheck->isChecked()) {
        add(mPhasesDatesUnfoldCheck);
    } else {
        mPhasesDatesUnfoldCheck->hide();
    }
    add(mPhasesStatCheck);

    totalH += phaseLayout->contentsMargins().top() + phaseLayout->contentsMargins().bottom();
    totalH += phaseLayout->spacing() * (mPhasesEventsUnfoldCheck->isChecked()? 7 : 6);

    replaceLayout(mPhasesGroup, phaseLayout);
    mPhasesGroup->setFixedHeight(totalH);
    optionWidgetHeight += totalH + 10;
}

void ResultsView::updateCurvesOptions(qreal &optionWidgetHeight)
{

#ifdef KOMLAN
    mGraphTypeTabs->setTabVisible(1, mLambdaRadio->isChecked() || mS02VgRadio->isChecked()); // history plot
    mGraphTypeTabs->setTabVisible(2, mLambdaRadio->isChecked() || mS02VgRadio->isChecked()); // acceptance Rate
    mGraphTypeTabs->setTabVisible(3, mLambdaRadio->isChecked() || mS02VgRadio->isChecked()); // auto- correlation
#else
    mGraphTypeTabs->setTabVisible(1, mLambdaRadio->isChecked()); // history plot
    mGraphTypeTabs->setTabVisible(2, mLambdaRadio->isChecked()); // acceptance Rate
    mGraphTypeTabs->setTabVisible(3, mLambdaRadio->isChecked()); // auto- correlation
#endif
    mEventsGroup->hide();
    mPhasesGroup->hide();
    mCurvesGroup->show();

    qreal totalH = 0;
    QVBoxLayout* curveLayout = new QVBoxLayout();
    curveLayout->setContentsMargins(10, 10, 10, 10);
    curveLayout->setSpacing(7);

    auto add = [&](QWidget* w) {
        curveLayout->addWidget(w);
        addTotalHeight(totalH, w);
    };

    add(mCurveGRadio);

    if (mCurveGRadio->isChecked()) {
        mCurveErrorCheck->show();
        mCurveHpdCheck->show();
        mCurveMapCheck->show();
        mCurveDataPointsCheck->show();
        mCurveEventsPointsCheck->show();

        QVBoxLayout* GLayout = new QVBoxLayout();
        GLayout->setContentsMargins(15, 0, 0, 0);
        GLayout->setSpacing(10);
        GLayout->addWidget(mCurveHpdCheck, Qt::AlignLeft);
        GLayout->addWidget(mCurveMapCheck, Qt::AlignLeft);
        GLayout->addWidget(mCurveDataPointsCheck, Qt::AlignLeft);
        GLayout->addWidget(mCurveErrorCheck, Qt::AlignLeft);
        GLayout->addWidget(mCurveEventsPointsCheck, Qt::AlignLeft);

        curveLayout->addLayout(GLayout);

        addTotalHeight(totalH, mCurveErrorCheck);
        addTotalHeight(totalH, mCurveHpdCheck);
        addTotalHeight(totalH, mCurveMapCheck) ;
        addTotalHeight(totalH, mCurveDataPointsCheck);
        addTotalHeight(totalH, mCurveEventsPointsCheck);

        totalH += GLayout->spacing() * 5;

    } else {
        mCurveErrorCheck->hide();
        mCurveHpdCheck->hide();
        mCurveMapCheck->hide();
        mCurveDataPointsCheck->hide();
        mCurveEventsPointsCheck->hide();
    }

    add(mCurveGPRadio);
    if (mCurveGPRadio->isChecked()) {

        mCurveGPHpdCheck->show();
        mCurveGPMapCheck->show();
        mCurveGPGaussCheck->show();

        QVBoxLayout* GLayout = new QVBoxLayout();
        GLayout->setContentsMargins(15, 0, 0, 0);
        GLayout->setSpacing(10);
        GLayout->addWidget(mCurveGPHpdCheck, Qt::AlignLeft);
        GLayout->addWidget(mCurveGPMapCheck, Qt::AlignLeft);
        GLayout->addWidget(mCurveGPGaussCheck, Qt::AlignLeft);

        curveLayout->addLayout(GLayout);

        addTotalHeight(totalH, mCurveGPHpdCheck);
        addTotalHeight(totalH, mCurveGPMapCheck);
        addTotalHeight(totalH, mCurveGPGaussCheck) ;

        totalH += GLayout->spacing() * 4;

    } else {
        mCurveGPHpdCheck->hide();
        mCurveGPMapCheck->hide();
        mCurveGPGaussCheck->hide();

    }
    add(mCurveGSRadio);
    add(mLambdaRadio);
#ifdef KOMLAN
    add(mS02VgRadio);
#endif
    add(mCurveStatCheck);

    totalH += curveLayout->contentsMargins().top() + curveLayout->contentsMargins().bottom();
    totalH += curveLayout->spacing() * (mCurveGRadio->isChecked()? 0: 2);
    replaceLayout(mCurvesGroup, curveLayout);
    mCurvesGroup->setFixedHeight(totalH);
    mOptionsLayout->addWidget(mCurvesGroup);
    optionWidgetHeight += totalH + 10;
}

void ResultsView::updateDisplayOptions(qreal &optionWidgetHeight)
{
    mDistribGroup->hide();
    mDisplayGroup->show();

    auto model = getModel_ptr();

    qreal totalH = 0.0;
    QVBoxLayout* dispLayout = new QVBoxLayout();
    dispLayout->setContentsMargins(10, 5, 10, 5);
    dispLayout->setSpacing(7);

    mDisplayStudyBut->setText(xScaleRepresentsTime() ? tr("Study Period Display") : tr("Fit Display"));

    // ---- SpanGroup = Time Scale
    mSpanTitle->show();
    addTotalHeight(totalH, mSpanTitle);

    mSpanGroup->show();

    QHBoxLayout* spanLayout0 = new QHBoxLayout();
    spanLayout0->setContentsMargins(0, 0, 0, 0);
    spanLayout0->addWidget(mDisplayStudyBut);

    addTotalHeight(totalH, mDisplayStudyBut);

    QHBoxLayout* spanLayout1 = new QHBoxLayout();
    spanLayout1->setContentsMargins(0, 0, 0, 0);
    spanLayout1->setSpacing(5);
    spanLayout1->addWidget(mCurrentTMinEdit);
    spanLayout1->addWidget(mSpanLab);
    spanLayout1->addWidget(mCurrentTMaxEdit);

    addTotalHeight(totalH, mCurrentTMinEdit);

    QHBoxLayout* spanLayout2 = new QHBoxLayout();
    spanLayout2->setContentsMargins(0, 0, 0, 0);
    spanLayout2->setSpacing(3);
    spanLayout2->addWidget(mTimeLab);
    spanLayout2->addWidget(mTimeSlider);
    spanLayout2->addWidget(mTimeEdit);

    addTotalHeight(totalH, mTimeEdit);

    QHBoxLayout* spanLayout3 = new QHBoxLayout();
    spanLayout3->setContentsMargins(0, 0, 0, 0);
    spanLayout3->setSpacing(5);
    spanLayout3->addWidget(mMajorScaleLab);
    spanLayout3->addWidget(mMajorScaleEdit);

    addTotalHeight(totalH, mMajorScaleEdit);

    QHBoxLayout* spanLayout4 = new QHBoxLayout();
    spanLayout4->setContentsMargins(0, 0, 0, 0);
    spanLayout4->setSpacing(5);
    spanLayout4->addWidget(mMinorScaleLab);
    spanLayout4->addWidget(mMinorScaleEdit);

    addTotalHeight(totalH, mMinorScaleEdit);

    QVBoxLayout* spanLayout = new QVBoxLayout();
    spanLayout->setContentsMargins(0, 0, 0, 0);
    spanLayout->setSpacing(10);
    spanLayout->addWidget(mSpanTitle);
    spanLayout->addLayout(spanLayout0);
    spanLayout->addLayout(spanLayout1);
    spanLayout->addLayout(spanLayout2);
    spanLayout->addLayout(spanLayout3);
    spanLayout->addLayout(spanLayout4);

    dispLayout->addLayout(spanLayout);

    totalH += spanLayout->spacing() * 6;


    const bool displayX = model ? model->displayX() : false;
    const bool displayY = model ? model->displayY() : false;
    const bool displayZ = model ? model->displayZ() : false;

    if (isCurve() && ( mMainVariable == GraphViewResults::eG ||
                      mMainVariable == GraphViewResults::eGP ||
                      mMainVariable == GraphViewResults::eGS)) {
        if (displayX) {
            mXOptionTitle->show();

            auto curveName = model->getCurvesName().at(0);
            if (mMainVariable == GraphViewResults::eGP) {
                curveName = tr("Var. ") + curveName;
            }  else if (mMainVariable == GraphViewResults::eGS) {
                curveName = tr("Acc. ") + curveName;
            }

            mXOptionTitle->setText(curveName + " " + tr("Scale"));
            mXOptionLab->setText(curveName);
            mXOptionBut->setText(tr("Optimal") + " " + curveName + " " + tr("Display"));


            addTotalHeight(totalH, mXOptionTitle);

            mXOptionGroup->show();
            mXOptionBut->show();
            mCurrentXMinEdit->show();
            mXOptionLab->show();
            mCurrentXMaxEdit->show();

            QHBoxLayout* XOptionLayout0 = new QHBoxLayout();
            XOptionLayout0->setContentsMargins(0, 0, 0, 0);
            XOptionLayout0->addWidget(mXOptionBut);
            addTotalHeight(totalH, mXOptionBut);

            QHBoxLayout* XOptionLayout1 = new QHBoxLayout();
            XOptionLayout1->setContentsMargins(0, 0, 0, 0);
            XOptionLayout1->setSpacing(5);
            XOptionLayout1->addWidget(mCurrentXMinEdit);
            XOptionLayout1->addWidget(mXOptionLab);
            XOptionLayout1->addWidget(mCurrentXMaxEdit);
            addTotalHeight(totalH, mCurrentXMaxEdit);

            QVBoxLayout* XOptionLayout = new QVBoxLayout();
            XOptionLayout->setContentsMargins(0, 0, 0, 0);
            XOptionLayout->setSpacing(10);
            XOptionLayout->addWidget(mXOptionTitle);
            XOptionLayout->addLayout(XOptionLayout0);
            XOptionLayout->addLayout(XOptionLayout1);

            addTotalHeight(totalH, mMajorScaleEdit);
            dispLayout->addLayout(XOptionLayout);

            totalH += XOptionLayout->spacing() * 3;

        } else {
            mXOptionTitle->hide();
            mXOptionGroup->hide();
            mXOptionBut->hide();
            mCurrentXMinEdit->hide();
            mXOptionLab->hide();
            mCurrentXMaxEdit->hide();
        }

        if (displayY) {
            mYOptionTitle->show();

            auto curveName = model->getCurvesName().at(1);
            if (mMainVariable == GraphViewResults::eGP) {
                curveName = tr("Var. ") + curveName;
            }  else if (mMainVariable == GraphViewResults::eGS) {
                curveName = tr("Acc. ") + curveName;
            }

            mYOptionTitle->setText(curveName + " " + tr("Scale"));
            mYOptionLab->setText(curveName);
            mYOptionBut->setText(tr("Optimal") + " " + curveName + " " + tr("Display"));


            addTotalHeight(totalH, mYOptionTitle);

            mYOptionGroup->show();
            mYOptionBut->show();
            mCurrentYMinEdit->show();
            mYOptionLab->show();
            mCurrentYMaxEdit->show();

            QHBoxLayout* YOptionLayout0 = new QHBoxLayout();
            YOptionLayout0->setContentsMargins(0, 0, 0, 0);
            YOptionLayout0->addWidget(mYOptionBut);
            addTotalHeight(totalH, mYOptionBut);

            QHBoxLayout* YOptionLayout1 = new QHBoxLayout();
            YOptionLayout1->setContentsMargins(0, 0, 0, 0);
            YOptionLayout1->setSpacing(5);
            YOptionLayout1->addWidget(mCurrentYMinEdit);
            YOptionLayout1->addWidget(mYOptionLab);
            YOptionLayout1->addWidget(mCurrentYMaxEdit);
            addTotalHeight(totalH, mCurrentYMaxEdit);

            QVBoxLayout* YOptionLayout = new QVBoxLayout();
            YOptionLayout->setContentsMargins(0, 0, 0, 0);
            YOptionLayout->setSpacing(10);
            YOptionLayout->addWidget(mYOptionTitle);
            YOptionLayout->addLayout(YOptionLayout0);
            YOptionLayout->addLayout(YOptionLayout1);

            dispLayout->addLayout(YOptionLayout);
            totalH += YOptionLayout->spacing() * 3;

        } else {
            mYOptionTitle->hide();
            mYOptionGroup->hide();
            mYOptionBut->hide();
            mCurrentYMinEdit->hide();
            mYOptionLab->hide();
            mCurrentYMaxEdit->hide();
        }
        if (displayZ) {
            mZOptionTitle->show();

            auto curveName = model->getCurvesName().at(2);
            if (mMainVariable == GraphViewResults::eGP) {
                curveName = tr("Var. ") + curveName;
            }  else if (mMainVariable == GraphViewResults::eGS) {
                curveName = tr("Acc. ") + curveName;
            }

            mZOptionTitle->setText(curveName + " " + tr("Scale"));
            mZOptionLab->setText(curveName);
            mZOptionBut->setText(tr("Optimal") + " " + curveName + " " + tr("Display"));

            addTotalHeight(totalH, mZOptionTitle);

            mZOptionGroup->show();
            mZOptionBut->show();
            mCurrentZMinEdit->show();
            mZOptionLab->show();
            mCurrentZMaxEdit->show();

            QHBoxLayout* ZOptionLayout0 = new QHBoxLayout();
            ZOptionLayout0->setContentsMargins(0, 0, 0, 0);
            ZOptionLayout0->addWidget(mZOptionBut);
            addTotalHeight(totalH, mZOptionBut);

            QHBoxLayout* ZOptionLayout1 = new QHBoxLayout();
            ZOptionLayout1->setContentsMargins(0, 0, 0, 0);
            ZOptionLayout1->setSpacing(5);
            ZOptionLayout1->addWidget(mCurrentZMinEdit);
            ZOptionLayout1->addWidget(mZOptionLab);
            ZOptionLayout1->addWidget(mCurrentZMaxEdit);
            addTotalHeight(totalH, mCurrentZMaxEdit);

            QVBoxLayout* ZOptionLayout = new QVBoxLayout();
            ZOptionLayout->setContentsMargins(0, 0, 0, 0);
            ZOptionLayout->setSpacing(10);
            ZOptionLayout->addWidget(mZOptionTitle);
            ZOptionLayout->addLayout(ZOptionLayout0);
            ZOptionLayout->addLayout(ZOptionLayout1);

            dispLayout->addLayout(ZOptionLayout);
            totalH += ZOptionLayout->spacing() * 3;

        } else {
            mZOptionTitle->hide();
            mZOptionGroup->hide();

            mZOptionBut->hide();
            mCurrentZMinEdit->hide();
            mZOptionLab->hide();
            mCurrentZMaxEdit->hide();
        }

    } else {
        mXOptionTitle->hide();
        mXOptionGroup->hide();
        mXOptionBut->hide();
        mCurrentXMinEdit->hide();
        mXOptionLab->hide();
        mCurrentXMaxEdit->hide();

        mYOptionTitle->hide();
        mYOptionGroup->hide();
        mYOptionBut->hide();
        mCurrentYMinEdit->hide();
        mYOptionLab->hide();
        mCurrentYMaxEdit->hide();

        mZOptionTitle->hide();
        mZOptionGroup->hide();
        mZOptionBut->hide();
        mCurrentZMinEdit->hide();
        mZOptionLab->hide();
        mCurrentZMaxEdit->hide();


    }

    mGraphicTitle->show();
    addTotalHeight(totalH, mGraphicTitle);

    mGraphicGroup->show();
    QHBoxLayout* graphicLayout1 = new QHBoxLayout();
    graphicLayout1->setContentsMargins(0, 0, 0, 0);
    graphicLayout1->setSpacing(3);
    graphicLayout1->addWidget(mZoomLab);
    graphicLayout1->addWidget(mZoomSlider);
    graphicLayout1->addWidget(mZoomEdit);
    addTotalHeight(totalH, mZoomEdit);

    QHBoxLayout* graphicLayout2 = new QHBoxLayout();
    graphicLayout2->setContentsMargins(0, 0, 0, 0);
    graphicLayout2->setSpacing(5);
    graphicLayout2->addWidget(mLabFont);
    graphicLayout2->addWidget(mFontBut);
    addTotalHeight(totalH, mFontBut);

    QHBoxLayout* graphicLayout3 = new QHBoxLayout();
    graphicLayout3->setContentsMargins(0, 0, 0, 0);
    graphicLayout3->setSpacing(3);
    graphicLayout3->addWidget(mLabThickness);
    graphicLayout3->addWidget(mThicknessCombo);
    addTotalHeight(totalH, mThicknessCombo);

    QHBoxLayout* graphicLayout4 = new QHBoxLayout();
    graphicLayout4->setContentsMargins(0, 0, 0, 0);
    graphicLayout4->setSpacing(3);
    graphicLayout4->addWidget(mLabOpacity);
    graphicLayout4->addWidget(mOpacityCombo);
    addTotalHeight(totalH, mOpacityCombo);

    QVBoxLayout* graphicLayout = new QVBoxLayout();
    graphicLayout->setContentsMargins(0, 0, 0, 0);
    graphicLayout->setSpacing(10);
    graphicLayout->addWidget(mGraphicTitle);
    graphicLayout->addLayout(graphicLayout1);
    graphicLayout->addLayout(graphicLayout2);
    graphicLayout->addLayout(graphicLayout3);
    graphicLayout->addLayout(graphicLayout4);
    totalH += graphicLayout->spacing() * 5;

    dispLayout->addLayout(graphicLayout);

    totalH += dispLayout->contentsMargins().top() + dispLayout->contentsMargins().bottom();
    replaceLayout(mDisplayGroup, dispLayout);

    mDisplayGroup->setFixedHeight(totalH);
    mOptionsLayout->addWidget(mDisplayGroup);
    optionWidgetHeight += totalH;
}

void ResultsView::updateDistribOptions(qreal &optionWidgetHeight, bool isPostDistrib)
{
    mDisplayGroup->hide();
    mDistribGroup->show();
    mChainsGroup->show();

    qreal totalH = 0.0;
    QVBoxLayout* distribLayout = new QVBoxLayout();
    distribLayout->setContentsMargins(10, 5, 10, 5);
    distribLayout->setSpacing(7);
    auto add = [&](QWidget* w) {
        w->show();
        distribLayout->addWidget(w);
        addTotalHeight(totalH, w);
    };

    add(mChainsTitle);

    if (isPostDistrib) {
        mAllChainsCheck->show();

        add(mAllChainsCheck);
        for (auto&& checkChain : mChainChecks) {
            add(checkChain);
        }
        for (auto&& chainRadio : mChainRadios) {
            chainRadio->hide();
        }
        totalH += distribLayout->spacing() * mChainChecks.size() + 2; // layout span

    } else {
        mAllChainsCheck->hide();
        for (auto&& checkChain : mChainChecks) {
            checkChain->hide();
        }
        for (auto&& chainRadio : mChainRadios) {
            add(chainRadio);
        }
        totalH += distribLayout->spacing() * mChainRadios.size() + 1; // layout span
    }


    if (isPostDistrib) {
        mDensityOptsTitle->show();
        mDensityOptsGroup->show();

        const QString tab = mGraphListTab->currentName();

        mCredibilityCheck->show();
        QVBoxLayout* densityLayout = new QVBoxLayout();
        densityLayout->setContentsMargins(0, 0, 0, 0);
        densityLayout->setSpacing(10);
        densityLayout->addWidget(mDensityOptsTitle);
        addTotalHeight(totalH, mDensityOptsTitle);

        densityLayout->addWidget(mCredibilityCheck);
        addTotalHeight(totalH, mCredibilityCheck);

        mThreshLab->show();
        mThresholdEdit->show();

        QHBoxLayout* densityLayout1 = new QHBoxLayout();
        densityLayout1->setContentsMargins(0, 0, 0, 0);
        densityLayout1->setSpacing(5);
        densityLayout1->addWidget(mThreshLab);
        densityLayout1->addWidget(mThresholdEdit);
        addTotalHeight(totalH, mThresholdEdit);
        densityLayout->addLayout(densityLayout1);

        mFFTLenLab->show();
        mFFTLenCombo->show();

        QHBoxLayout* densityLayout2 = new QHBoxLayout();
        densityLayout2->setContentsMargins(0, 0, 0, 0);
        densityLayout2->setSpacing(5);
        densityLayout2->addWidget(mFFTLenLab);
        densityLayout2->addWidget(mFFTLenCombo);
        addTotalHeight(totalH, mFFTLenCombo);
        densityLayout->addLayout(densityLayout2);

        mBandwidthLab->show();
        mBandwidthEdit->show();
        QHBoxLayout* densityLayout3 = new QHBoxLayout();
        densityLayout3->setContentsMargins(0, 0, 0, 0);
        densityLayout3->setSpacing(5);
        densityLayout3->addWidget(mBandwidthLab);
        densityLayout3->addWidget(mBandwidthEdit);
        addTotalHeight(totalH, mBandwidthEdit);
        densityLayout->addLayout(densityLayout3);

        totalH += densityLayout->spacing() * 3;
        distribLayout->addLayout(densityLayout);

        if (tab == tr("Phases") && mActivityRadio->isChecked()) {
            mActivityOptsTitle->show();
            mActivityOptsGroup->show();

            mHActivityLab->show();
            mHActivityEdit->show();

            mRangeThreshLab->show();
            mRangeThresholdEdit->show();

            QVBoxLayout* activityLayout = new QVBoxLayout();
            activityLayout->setContentsMargins(0, 0, 0, 0);
            activityLayout->setSpacing(10);
            activityLayout->addWidget(mActivityOptsTitle);
            addTotalHeight(totalH, mActivityOptsTitle);

            QHBoxLayout* HActivitiyLayout = new QHBoxLayout();
            HActivitiyLayout->setContentsMargins(0, 0, 0, 0);
            HActivitiyLayout->setSpacing(5);
            HActivitiyLayout->addWidget(mHActivityLab);
            HActivitiyLayout->addWidget(mHActivityEdit);
            addTotalHeight(totalH, mHActivityEdit);
            activityLayout->addLayout(HActivitiyLayout);

            QHBoxLayout* rangeLayout = new QHBoxLayout();
            rangeLayout->setContentsMargins(0, 0, 0, 0);
            rangeLayout->setSpacing(5);
            rangeLayout->addWidget(mRangeThreshLab);
            rangeLayout->addWidget(mRangeThresholdEdit);
            addTotalHeight(totalH, mRangeThresholdEdit);
            activityLayout->addLayout(rangeLayout);

            totalH += activityLayout->spacing() * 2;
            distribLayout->addLayout(activityLayout);

        } else {
            mActivityOptsTitle->hide();
            mActivityOptsGroup->hide();
            mHActivityLab->hide();
            mHActivityEdit->hide();
            mRangeThreshLab->hide();
            mRangeThresholdEdit->hide();
        }

        totalH += distribLayout->spacing() * (tab == tr("Phases")? 5: 4); // layout span
        totalH += distribLayout->contentsMargins().top() + distribLayout->contentsMargins().bottom();
    } else {
        mDensityOptsTitle->hide();
        mDensityOptsGroup->hide();

        mCredibilityCheck->hide();

        mRangeThreshLab->hide();
        mRangeThresholdEdit->hide();

        mThreshLab->hide();
        mThresholdEdit->hide();

        mHActivityLab->hide();
        mHActivityEdit->hide();

        mFFTLenLab->hide();
        mFFTLenCombo->hide();

        mBandwidthLab->hide();
        mBandwidthEdit->hide();
    }


    totalH += distribLayout->contentsMargins().top() + distribLayout->contentsMargins().bottom();
    replaceLayout(mDistribGroup, distribLayout);
    mDistribGroup->setFixedHeight(totalH);
    mOptionsLayout->addWidget(mDistribGroup);
    optionWidgetHeight += totalH;
}

void ResultsView::updatePageSaveOptions(qreal &optionWidgetHeight)
{
    mPageWidget->hide();

    qreal totalH = 0.0;

    QVBoxLayout* saveLayout = new QVBoxLayout();
    saveLayout->setContentsMargins(0, 5, 0, 5);
    saveLayout->setSpacing(0);

    if (currentGraphs(true).isEmpty()) {
        mSaveSelectWidget->hide();
        mSaveAllWidget->show();
        mSaveAllWidget->setFixedHeight(66);

        saveLayout->addWidget(mSaveAllWidget);
        addTotalHeight(totalH, mSaveAllWidget);

    } else {
        mSaveAllWidget->hide();
        mSaveSelectWidget->show();
        mSaveSelectWidget->setFixedHeight(66);

        saveLayout->addWidget(mSaveSelectWidget);
        addTotalHeight(totalH, mSaveSelectWidget);

    }
    totalH += saveLayout->contentsMargins().top() + saveLayout->contentsMargins().bottom();
    replaceLayout(mPageSaveGroup, saveLayout);
    mPageSaveGroup->setFixedHeight(totalH);

    mOptionsLayout->addWidget(mPageSaveGroup);
    optionWidgetHeight += totalH;
}

void ResultsView::updateOptionsWidget()
{
    auto model = getModel_ptr();
    if (model ==  nullptr)
        return;

    if (!model || !mOptionsWidget) return;

    bool isPostDistrib = isPostDistribGraph();
    qreal optionWidgetHeight = 0;

    delete mOptionsLayout;
    mOptionsLayout = new QVBoxLayout();
    mOptionsLayout->setContentsMargins(0, 0, 0, 0);
    mOptionsLayout->setSpacing(0); // espace ente les menus d'option

    mOptionsLayout->addWidget(mGraphListTab);
    optionWidgetHeight += mGraphListTab->height();

    const QString tab = mGraphListTab->currentName();

    if (tab == tr("Events")) {
        updateEventsOptions(optionWidgetHeight, isPostDistrib);

    }
    else if (tab == tr("Phases")) {
        updatePhasesOptions(optionWidgetHeight);

    }
    else if (tab == tr("Curves")) {
        updateCurvesOptions(optionWidgetHeight);

    }

    // ------------------------------------
    //  Display / Distrib. Option
    // ------------------------------------
    mOptionsLayout->addWidget(mDisplayDistribTab);
    optionWidgetHeight += mDisplayDistribTab->height();

    const auto tab_display = mDisplayDistribTab->currentIndex();

    if (tab_display == 0) {
        updateDisplayOptions(optionWidgetHeight);

    }
    else if (tab_display == 1) {
        updateDistribOptions(optionWidgetHeight, isPostDistrib);

    }

    // -------------------------------------------------------------------------------------
    //  Page / Save
    // -------------------------------------------------------------------------------------

    mOptionsLayout->addWidget(mPageSaveTab);
    optionWidgetHeight += mPageSaveTab->height();
    mPageSaveGroup->show();
    const QString tab_page = mPageSaveTab->currentName();

    if (tab_page == tr("Page")) {
        mSaveSelectWidget->hide();
        mSaveAllWidget->hide();

        mPageWidget->show();
        updateTotalGraphs();
        const int numPages = ceil((double)mMaximunNumberOfVisibleGraph / (double)mGraphsPerPage);
        if (mCurrentPage >= numPages) {
            mCurrentPage = 0;
        }

        mPageEdit->setText(QLocale().toString(mCurrentPage + 1) + "/" + QLocale().toString(numPages));
        mPageWidget->setFixedHeight(66);

        QVBoxLayout* saveLayout = new QVBoxLayout();
        saveLayout->setContentsMargins(0, 5, 0, 5);

        saveLayout->addWidget(mPageWidget);
        addTotalHeight(optionWidgetHeight, mPageWidget);

        replaceLayout(mPageSaveGroup, saveLayout);
        mOptionsLayout->addWidget(mPageSaveGroup);
    }
    else  {
        updatePageSaveOptions(optionWidgetHeight);

    }

    mOptionsWidget->setLayout(mOptionsLayout);
    // Ajustement final de la hauteur
    mOptionsWidget->setFixedHeight(optionWidgetHeight + 20);

    mOptionsWidget->setGeometry(0, 0, mOptionsW - mMargin, optionWidgetHeight +20);
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
    double origin = mHeightForVisibleAxis;

    const double prop = QLocale().toDouble(mZoomEdit->text()) / 100.0;
    mGraphHeight = min + prop * (origin - min);
    updateGraphsLayout();
}

void ResultsView::updateZoomT()
{
    // Pick the value from th spin or the slider
    const double zoom = QLocale().toDouble(mTimeEdit->text())/100.0;

    mResultZoomT = 1./zoom;

    const double tCenter = (mResultCurrentMaxT + mResultCurrentMinT)/2.0;
    const double span = (mResultMaxT - mResultMinT)* (1.0/ zoom);

    double curMin = tCenter - span/2.;
    double curMax = tCenter + span/2.;

    if (curMin < mRuler->mMin) {
        curMin = mRuler->mMin;
        curMax = curMin + span;

    } else if (curMax > mRuler->mMax) {
        curMax = mRuler->mMax;
        curMin = curMax - span;
    }

    mResultCurrentMinT = curMin;
    mResultCurrentMaxT = curMax;

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

        mZoomsT[key] = QPair<double, double>(resultMinMax.first, resultMinMax.second);

    } else {
        mZoomsT[key] = QPair<double, double>(mResultCurrentMinT, mResultCurrentMaxT);
    }

    mScalesT[key] = QPair<double, int>(mMajorScale, mMinorCountScale);
}

#pragma mark Controls setters

void ResultsView::setTimeRange()
{
    mCurrentTMinEdit->resetText(mResultCurrentMinT);
    mCurrentTMaxEdit->resetText(mResultCurrentMaxT);

}

void ResultsView::setTimeEdit(const double value)
{
    mTimeEdit->resetText(value * 100.0);

}

void ResultsView::setTimeSlider(const int value)
{
    mTimeSlider->blockSignals(true);
    mTimeSlider->setValue(value);
    mTimeSlider->blockSignals(false);
}

void ResultsView::setTimeScale()
{
    QLocale locale = QLocale();
    mMinorScaleEdit->setText(locale.toString(mMinorCountScale));
    mMajorScaleEdit->setText(locale.toString(mMajorScale));

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
#ifdef KOMLAN
             mMainVariable == GraphViewResults::eS02Vg ||
#endif
             mMainVariable == GraphViewResults::eVg ) ) {
                mResultCurrentMinT = std::max(min, mResultMinT);
                mResultCurrentMaxT = max;

            } else {
                mResultCurrentMinT = min;
                mResultCurrentMaxT = max;
            }

    }
    setTimeRange();
    updateGraphsZoomT();
}

void ResultsView::applyStudyPeriod()
{
    auto model = getModel_ptr();
    if (xScaleRepresentsTime()) {
      mResultCurrentMinT = model->mSettings.getTminFormated();
      mResultCurrentMaxT = model->mSettings.getTmaxFormated();

    } else if ( mMainVariable == GraphViewResults::eSigma ||
                mMainVariable == GraphViewResults::eDuration ||
#ifdef S02_BAYESIAN
                mMainVariable == GraphViewResults::eS02  ||
#endif

#ifdef KOMLAN
                mMainVariable == GraphViewResults::eS02Vg  ||
#endif
                mMainVariable == GraphViewResults::eVg ) {
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
        for (const auto& chRadio : std::as_const(mChainRadios)) {
               if (chRadio->isChecked())
                      break;
               ++idSelect;
        }
        const ChainSpecs& chain = model->mChains.at(idSelect);
        const int adaptSize = chain.mBatchIndex * chain.mIterPerBatch;
        const int runSize = chain.mRealyAccepted;
        // The min is always 0
        mResultCurrentMinT = 0.;
        mResultCurrentMaxT = 1 +  chain.mIterPerBurn + adaptSize + runSize;

    }
        
    if (xScaleRepresentsTime()) {
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
    setTimeEdit(mResultZoomT);
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
    double min = QLocale().toDouble(minStr, &minIsNumber);

    QString maxStr = mCurrentTMaxEdit->text();
    bool maxIsNumber = true;
    double max = QLocale().toDouble(maxStr, &maxIsNumber);

    if (minIsNumber && maxIsNumber && ((min != mResultCurrentMinT) || (max != mResultCurrentMaxT))) {
        mResultCurrentMinT = std::max(min, mRuler->mMin);
        mResultCurrentMaxT = std::min(max, mRuler->mMax);

        mResultZoomT = (mResultMaxT - mResultMinT)/ (mResultCurrentMaxT - mResultCurrentMinT);

        mRuler->setCurrent(mResultCurrentMinT, mResultCurrentMaxT);

        updateGraphsZoomT();

        setTimeRange();
        setTimeSlider(zoomToSlider(mResultZoomT));
        setTimeEdit(mResultZoomT);
    }
}

void ResultsView::applyTimeSlider(int value)
{
    setTimeEdit(sliderToZoom(value));
    updateZoomT();
}

void ResultsView::applyTimeEdit()
{
    if (mTimeEdit->hasAcceptableInput()) {
        setTimeSlider(zoomToSlider(QLocale().toDouble(mTimeEdit->text()) / 100.0));
        updateZoomT();
    }

}

# pragma mark Curve Zoom
void ResultsView::applyXRange()
{
    bool minIsNumber = true;
    const double minX = QLocale().toDouble(mCurrentXMinEdit->text(), &minIsNumber);

    bool maxIsNumber = true;
    const double maxX = QLocale().toDouble(mCurrentXMaxEdit->text(), &maxIsNumber);
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
    const double minY = QLocale().toDouble(mCurrentYMinEdit->text(), &minIsNumber);

    bool maxIsNumber = true;
    const double maxY = QLocale().toDouble(mCurrentYMaxEdit->text(), &maxIsNumber);
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
    const double minZ = QLocale().toDouble(mCurrentZMinEdit->text(), &minIsNumber);

    bool maxIsNumber = true;
    const double maxZ = QLocale().toDouble(mCurrentZMaxEdit->text(), &maxIsNumber);
    if (minIsNumber && maxIsNumber && minZ< maxZ) {
        mResultCurrentMinZ = minZ;
        mResultCurrentMaxZ = maxZ;
        setZRange();

        updateCurvesToShow();
    }

}


void ResultsView::findOptimalX()
{
    const auto model = getModel_ptr();
    const std::vector<double>* vec = nullptr;

    Scale XScale;
    if (mCurveGRadio->isChecked()) {
        const auto minmax_Y = model->mPosteriorMeanG.gx.mapG.minMaxY();
        vec = &model->mPosteriorMeanG.gx.vecG;
        const std::vector<double>* vecVar = &model->mPosteriorMeanG.gx.vecVarG;

        double minY = +std::numeric_limits<double>::max();
        double maxY = -std::numeric_limits<double>::max();
        minY = std::accumulate(model->mEvents.begin(), model->mEvents.end(), minY, [](double x, std::shared_ptr<Event> e) {return std::min(e->mXIncDepth, x);});
        maxY = std::accumulate(model->mEvents.begin(), model->mEvents.end(), maxY, [](double x, std::shared_ptr<Event> e) {return std::max(e->mXIncDepth, x);});
        int i = 0;
        for (auto g : *vec) {
            const auto e = 1.96*sqrt(vecVar->at(i));
            minY = std::min(minY, g - e);
            maxY = std::max(maxY, g + e);
            i++;
        }

        XScale.findOptimal(std::min(minmax_Y.first, minY), std::max(minmax_Y.second, maxY), 7);

    } else {
        if (mCurveGPRadio->isChecked()) {
            const auto minmax_Y = model->mPosteriorMeanG.gx.mapGP.minMaxY();
            vec = &model->mPosteriorMeanG.gx.vecGP;
            const auto minMax = std::minmax_element(vec->begin(), vec->end());
            XScale.findOptimal(std::min(minmax_Y.first, *minMax.first), std::max(minmax_Y.second, *minMax.second), 7);

        } else {
            vec = &model->mPosteriorMeanG.gx.vecGS;
            const auto minMax = std::minmax_element(vec->begin(), vec->end());
            XScale.findOptimal(*minMax.first, *minMax.second, 7);
        }

    }


    mResultCurrentMinX = XScale.min;
    mResultCurrentMaxX = XScale.max;

    setXRange();

    updateCurvesToShow();

}

void ResultsView::findOptimalY()
{
    const auto model = getModel_ptr();
    const std::vector<double>* vec = nullptr;

    Scale XScale;
    if (mCurveGRadio->isChecked()) {
        vec = &model->mPosteriorMeanG.gy.vecG;
        const std::vector<double>* vecVar = &model->mPosteriorMeanG.gy.vecVarG;

        double minY = +std::numeric_limits<double>::max();
        double maxY = -std::numeric_limits<double>::max();
        minY = std::accumulate(model->mEvents.begin(), model->mEvents.end(), minY, [](double x, std::shared_ptr<Event> e) {return std::min(e->mYDec, x);});
        maxY = std::accumulate(model->mEvents.begin(), model->mEvents.end(), maxY, [](double x, std::shared_ptr<Event> e) {return std::max(e->mYDec, x);});
        int i = 0;
        for (auto g : *vec) {
            const auto e = 1.96*sqrt(vecVar->at(i));
            minY = std::min(minY, g - e);
            maxY = std::max(maxY, g + e);
            i++;
        }

        XScale.findOptimal(minY, maxY, 7);

    } else {
        if (mCurveGPRadio->isChecked()) {
            const auto minmax_Y = model->mPosteriorMeanG.gy.mapGP.minMaxY();
            vec = &model->mPosteriorMeanG.gy.vecGP;
            const auto minMax = std::minmax_element(vec->begin(), vec->end());
            XScale.findOptimal(std::min(minmax_Y.first, *minMax.first), std::max(minmax_Y.second, *minMax.second), 7);

        } else {
            vec = &model->mPosteriorMeanG.gy.vecGS;
            const auto minMax = std::minmax_element(vec->begin(), vec->end());
            XScale.findOptimal(*minMax.first, *minMax.second, 7);
        }
    }


    mResultCurrentMinY = XScale.min;
    mResultCurrentMaxY = XScale.max;
    setYRange();

    updateCurvesToShow();
}

void ResultsView::findOptimalZ()
{
    const auto model = getModel_ptr();

    const std::vector<double>* vec = nullptr;
    Scale XScale;
    if (mCurveGRadio->isChecked()) {
        vec = &model->mPosteriorMeanG.gz.vecG;
        const std::vector<double>* vecVar = &model->mPosteriorMeanG.gz.vecVarG;

        double minY = +std::numeric_limits<double>::max();
        double maxY = -std::numeric_limits<double>::max();
        minY = std::accumulate(model->mEvents.begin(), model->mEvents.end(), minY, [](double x, std::shared_ptr<Event> e) {return std::min(e->mZField, x);});
        maxY = std::accumulate(model->mEvents.begin(), model->mEvents.end(), maxY, [](double x, std::shared_ptr<Event> e) {return std::max(e->mZField, x);});
        int i = 0;
        for (auto g : *vec) {
            const auto e = 1.96*sqrt(vecVar->at(i));
            minY = std::min(minY, g - e);
            maxY = std::max(maxY, g + e);
            i++;
        }

        XScale.findOptimal(minY, maxY, 7);

    } else {
        if (mCurveGPRadio->isChecked()) {
            const auto minmax_Y = model->mPosteriorMeanG.gz.mapGP.minMaxY();
            vec = &model->mPosteriorMeanG.gz.vecGP;
            const auto minMax = std::minmax_element(vec->begin(), vec->end());
            XScale.findOptimal(std::min(minmax_Y.first, *minMax.first), std::max(minmax_Y.second, *minMax.second), 7);

        } else {
            vec = &model->mPosteriorMeanG.gz.vecGS;
            const auto minMax = std::minmax_element(vec->begin(), vec->end());
            XScale.findOptimal(*minMax.first, *minMax.second, 7);
        }

    }

    mResultCurrentMinZ = XScale.min;
    mResultCurrentMaxZ = XScale.max;

    setZRange();
    updateCurvesToShow();
}

void ResultsView::setXRange()
{
    mCurrentXMinEdit->resetText(mResultCurrentMinX);
    mCurrentXMaxEdit->resetText(mResultCurrentMaxX);

    QPair<GraphViewResults::variable_t, GraphViewResults::graph_t> key(mMainVariable, mCurrentTypeGraph);
    mZoomsX[key] = QPair<double, double>(mResultCurrentMinX, mResultCurrentMaxX);
}

void ResultsView::setYRange()
{
    mCurrentYMinEdit->resetText(mResultCurrentMinY);
    mCurrentYMaxEdit->resetText(mResultCurrentMaxY);

    QPair<GraphViewResults::variable_t, GraphViewResults::graph_t> key(mMainVariable, mCurrentTypeGraph);
    mZoomsY[key] = QPair<double, double>(mResultCurrentMinY, mResultCurrentMaxY);
}

void ResultsView::setZRange()
{

    mCurrentZMinEdit->resetText(mResultCurrentMinZ);
    mCurrentZMaxEdit->resetText(mResultCurrentMaxZ);

    QPair<GraphViewResults::variable_t, GraphViewResults::graph_t> key(mMainVariable, mCurrentTypeGraph);
    mZoomsZ[key] = QPair<double, double>(mResultCurrentMinZ, mResultCurrentMaxZ);
}

void ResultsView::applyZoomScale()
{
    QString majorStr = mMajorScaleEdit->text();
    bool isMajorNumber = true;
    double majorNumber = QLocale().toDouble(majorStr, &isMajorNumber);
    if (!isMajorNumber)
        return;

    QString minorStr = mMinorScaleEdit->text();
    bool isMinorNumber = true;
    double minorNumber = QLocale().toDouble(minorStr, &isMinorNumber);
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
    mScalesT[key] = QPair<double, int>(mMajorScale, mMinorCountScale);
}

#pragma mark Graphics Option Zoom
void ResultsView::applyZoomSlider(int value)
{
    mZoomEdit->resetText(value);

    /*mZoomEdit->blockSignals(true);
    mZoomEdit->setText(QLocale().toString(value));
    mZoomEdit->blockSignals(false);*/

    QPair<GraphViewResults::variable_t, GraphViewResults::graph_t> key(mMainVariable, mCurrentTypeGraph);
    mZoomsH[key] = value;

    updateGraphsHeight();
}

void ResultsView::applyZoomEdit()
{
    if (mZoomEdit->hasAcceptableInput()) {

        mZoomSlider->blockSignals(true);
        mZoomSlider->setValue(QLocale().toInt(mZoomEdit->text()));
        mZoomSlider->blockSignals(false);

        QPair<GraphViewResults::variable_t, GraphViewResults::graph_t> key(mMainVariable, mCurrentTypeGraph);
        mZoomsH[key] = mZoomSlider->value();
        updateGraphsHeight();
    }

}

void ResultsView::applyFont()
{
    bool ok = false;

    const QFont& currentFont = mFontBut->font();

    const QFont font(QFontDialog::getFont(&ok, currentFont, this));

    if (ok) {
        const QList<GraphViewResults*> &graphs = allGraphs();
        for (const auto& graph : graphs) {
            graph->setGraphsFont(font);
            graph->update();
        }

        mFontBut->setText(font.family() + ", " + font.styleName() + ", " +QString::number(font.pointSizeF()));
        mFontBut->setFont(font);

    }
}

void ResultsView::applyThickness(int value)
{
    const QList<GraphViewResults*>& graphs = allGraphs();
    for (const auto& graph : graphs) {
        graph->updateCurvesThickness(value);
    }
    generateCurves();//updateGraphsLayout();
}

void ResultsView::applyOpacity(int value)
{
    const int opValue = value * 10;
    const QList<GraphViewResults*> &graphs = allGraphs();
    for (const auto& graph : graphs) {
        graph->setCurvesOpacity(opValue);
    }
    generateCurves();
}


void ResultsView::setGraphicOption(GraphViewResults& graph)
{
    auto model = getModel_ptr();
    graph.setSettings(model->mSettings);
    graph.setMCMCSettings(model->mMCMCSettings, model->mChains);
    graph.setGraphsFont(mFontBut->font());
    graph.setCurvesThickness(mThicknessCombo->currentIndex());
    graph.setCurvesOpacity(mOpacityCombo->currentIndex()*10);

    graph.changeXScaleDivision(mMajorScale, mMinorCountScale);  
    graph.setMarginLeft(mMarginLeft);
    graph.setMarginRight(mMarginRight);

}


void ResultsView::applyFFTLength()
{
    const int len = mFFTLenCombo->currentText().toInt();
    getModel_ptr()->setFFTLength(len);
    generateCurves();
}

void ResultsView::applyHActivity()
{
    if (mRangeThresholdEdit->hasAcceptableInput()) {
        const double h = QLocale().toDouble(mHActivityEdit->text());
        const double rangePercent = QLocale().toDouble(mRangeThresholdEdit->text());
        getModel_ptr()->setHActivity(h, rangePercent);
        generateCurves();
    }

}

void ResultsView::applyBandwidth()
{
   if (mBandwidthEdit->hasAcceptableInput()) {
        const double bandwidth = QLocale().toDouble(mBandwidthEdit->text());
        getModel_ptr()->setBandwidth(bandwidth);
        generateCurves();
    }
}

void ResultsView::applyThreshold()
{
    if (mThresholdEdit->hasAcceptableInput()) {
        const double threshold = QLocale().toDouble(mThresholdEdit->text());

        mHpdThreshold = threshold;
        auto threshold_str = stringForLocal(mHpdThreshold) + "%";
        mCurveGRadio->setText(tr("Curve (at %1% Level)").arg(threshold_str));

        getModel_ptr()->setThreshold(threshold);
#pragma mark TODO
        if (isCurve()) {
            updateCurveEventsPointX();
            //updateCurveEventsPointY(); //todo
            //updateCurveEventsPointZ();
        }
        generateCurves();
    }
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
    mEventsStatCheck->setChecked(show);
    mPhasesStatCheck->setChecked(show);
    mCurveStatCheck->setChecked(show);
    
    QList<GraphViewResults*> graphs = allGraphs();
    for (GraphViewResults*& graph : graphs) {
        graph->showNumericalResults(show);
    }
    updateLayout();
}

#pragma mark Graph selection and export

void ResultsView::saveGraphData()
{
    QList<GraphViewResults*> graphs = currentGraphs(true);
    for (auto&& graph : graphs) {
        graph->saveGraphData(mHpdThreshold);
    }
}

void ResultsView::resultsToClipboard()
{
    QString resultText;
    QList<GraphViewResults*> graphs = currentGraphs(true);
    for (const auto& graph : std::as_const(graphs)) {
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

        QImage image(firstGraph->width() * pr, ((int)graphs.size() * firstGraph->height() + versionHeight) * pr , QImage::Format_ARGB32_Premultiplied);

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

            QImage image (graphs.first()->width() * pr, ((int)graphs.size() * graphs.first()->height() + heightText) * pr , QImage::Format_ARGB32_Premultiplied); //Format_ARGB32_Premultiplied //Format_ARGB32

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
            const int hGraph = (int)graphs.size() * graphs.first()->height();
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
    auto model = getModel_ptr();
    if (model) {

        const QString csvSep = AppSettings::mCSVCellSeparator;
        const int precision = AppSettings::mPrecision;
        QLocale csvLocal = AppSettings::mCSVDecSeparator == "." ? QLocale::English : QLocale::French;

        csvLocal.setNumberOptions(QLocale::OmitGroupSeparator);

        const QString currentPath = MainWindow::getInstance()->getCurrentPath();
        const QString dirPath = QFileDialog::getSaveFileName(qApp->activeWindow(),
                                                        tr("Export to directory..."),
                                                       currentPath);//,
                                                      // tr("Directory"));


        if (!dirPath.isEmpty()) {
            QDir dir(dirPath);
            if (dir.exists()) {
                /*if(QMessageBox::question(qApp->activeWindow(), tr("Are you sure?"), tr("This directory already exists and all its content will be deleted. Do you really want to replace it?")) == QMessageBox::No){
                    return;
                }*/
                dir.removeRecursively();
            }
            dir.mkpath("."); // Crée le répertoire

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
                output<<model->getModelLog();

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
                output<<model->getInitLog();

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
                output<<model->getAdaptLog();

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
                output<<model->getResultsLog();

                output<<"</body>"<< Qt::endl;
                output<<"</html>"<< Qt::endl;
            }
            file.close();

            const QList<QStringList> stats = model->getStats(csvLocal, precision, true);
            saveCsvTo(stats, dirPath + "/Synthetic_Stats_Table.csv", csvSep, true);

            if (model->mPhases.size() > 0) {
                const QList<QStringList> phasesTraces = model->getPhasesTraces(csvLocal, false);
                saveCsvTo(phasesTraces, dirPath + "/Chain_all_Phases.csv", csvSep, false);

                for (size_t i=0; i<model->mPhases.size(); ++i) {
                    const QList<QStringList> phaseTrace = model->getPhaseTrace(i, csvLocal, false);
                    const QString name = model->mPhases.at(i)->getQStringName().toLower().simplified().replace(" ", "_");
                    saveCsvTo(phaseTrace, dirPath + "/Chain_Phase_" + name + ".csv", csvSep, false);
                }
            }
            QList<QStringList> eventsTraces = model->getEventsTraces(csvLocal, false);
            saveCsvTo(eventsTraces, dirPath + "/Chain_all_Events.csv", csvSep, false);

            // --------------   Saving Curve Map
            if (getProject_ptr()->isCurve()) {
                // first Map G
                const auto list_names = model->getCurvesName();

                file.setFileName(dirPath + "/Curve_" + list_names.at(0) + "_Map.csv");

                if (file.open(QFile::WriteOnly | QFile::Truncate)) {
                    model->saveMapToFile(&file, csvSep, model->mPosteriorMeanG.gx.mapG);

                }

                if (model->displayY()) {
                    file.setFileName(dirPath + "/Curve" + list_names.at(1) + "_Map.csv");

                    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
                        model->saveMapToFile(&file, csvSep, model->mPosteriorMeanG.gy.mapG);
                        file.close();
                    }

                    if (model->displayZ()) {
                        file.setFileName(dirPath + "/Curve" + list_names.at(2) + "_Map.csv");

                        if (file.open(QFile::WriteOnly | QFile::Truncate)) {
                            model->saveMapToFile(&file, csvSep, model->mPosteriorMeanG.gz.mapG);
                            file.close();
                        }
                    }

                }

                // --------------   Saving Curve Ref


                model->exportMeanGComposanteToReferenceCurves(model->mPosteriorMeanG.gx, dirPath + "/Curve_" + list_names.at(0) + "_Gauss_ref.csv", QLocale::English, ",");
                model->exportMeanGPComposanteToReferenceCurves(model->mPosteriorMeanG.gx,dirPath +  "/Curve_" + list_names.at(0) + "_GP_Gauss_ref.csv", QLocale::English, ",");

                model->exportHpdGComposanteToReferenceCurves(model->mPosteriorMeanG.gx, dirPath + "/Curve_" + list_names.at(0) + "_Hpd_ref.csv", QLocale::English, ",");
                model->exportHpdGPComposanteToReferenceCurves(model->mPosteriorMeanG.gx, dirPath + "/Curve_" + list_names.at(0) + "_GP_Hpd_ref.csv", QLocale::English, ",");

                // Second Map GP

                file.setFileName(dirPath + "/Curve_" + list_names.at(0) + "_GP_Map.csv");

                if (file.open(QFile::WriteOnly | QFile::Truncate)) {
                    model->saveMapToFile(&file, csvSep, model->mPosteriorMeanG.gx.mapGP);

                }


                if (model->displayY()) {
                    model->exportMeanGComposanteToReferenceCurves(model->mPosteriorMeanG.gy,dirPath +  "/Curve_" + list_names.at(1) + "_Gauss_ref.csv", QLocale::English, ",");
                    model->exportMeanGPComposanteToReferenceCurves(model->mPosteriorMeanG.gy,  dirPath + "/Curve_" + list_names.at(1) + "_GP_Gauss_ref.csv", QLocale::English, ",");

                    model->exportHpdGComposanteToReferenceCurves(model->mPosteriorMeanG.gy, dirPath + "/Curve_" + list_names.at(0) + "__Hpd_ref.csv", QLocale::English, ",");
                    model->exportHpdGPComposanteToReferenceCurves(model->mPosteriorMeanG.gy, dirPath + "/Curve_" + list_names.at(0) + "_GP_Hpd_ref.csv", QLocale::English, ",");


                    file.setFileName(dirPath + "/Curve" + list_names.at(1) + "_GP_Map.csv");

                    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
                        model->saveMapToFile(&file, csvSep, model->mPosteriorMeanG.gy.mapGP);
                        file.close();
                    }

                    if (model->displayZ()) {
                        model->exportMeanGComposanteToReferenceCurves(model->mPosteriorMeanG.gz, dirPath +"/Curve_" + list_names.at(2) + "_Gauss_ref.csv", QLocale::English, ",");
                        model->exportMeanGPComposanteToReferenceCurves(model->mPosteriorMeanG.gz, dirPath +"/Curve_" + list_names.at(2) + "_GP_Gauss_ref.csv", QLocale::English, ",");

                        model->exportHpdGComposanteToReferenceCurves(model->mPosteriorMeanG.gz, dirPath +"/Curve_" + list_names.at(0) + "_Hpd_ref.csv", QLocale::English, ",");
                        model->exportHpdGPComposanteToReferenceCurves(model->mPosteriorMeanG.gz, dirPath +"/Curve_" + list_names.at(0) + "_GP_Hpd_ref.csv", QLocale::English, ",");


                        file.setFileName(dirPath + "/Curve" + list_names.at(2) + "_GP_Map.csv");

                        if (file.open(QFile::WriteOnly | QFile::Truncate)) {
                            model->saveMapToFile(&file, csvSep, model->mPosteriorMeanG.gz.mapGP);
                            file.close();
                        }
                    }

                }

            }
        }
    }
}

void ResultsView::exportFullImage()
{
    bool printAxis = (mGraphHeight < mHeightForVisibleAxis);

    QWidget* curWid (nullptr);

    if (mGraphListTab->currentName() == tr("Events")) {
        curWid = mEventsScrollArea->widget();
        curWid->setFont(mByEventsGraphs.at(0)->font());

    } else if (mGraphListTab->currentName() == tr("Phases")) {
        curWid = mPhasesScrollArea->widget();
        curWid->setFont(mByPhasesGraphs.at(0)->font());

    } else if (mGraphListTab->currentName() == tr("Curves")) {
        curWid = mCurvesScrollArea->widget();
        curWid->setFont(mByCurvesGraphs.at(0)->font());


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

        if (mEventsStatCheck->isChecked()) { // all StatCheck are connected
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
        if (mEventsStatCheck->isChecked())
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
            axisWidget = nullptr;
        }
        if (axisLegend) {
            axisLegend->setParent(nullptr);
            delete axisLegend;
            axisLegend = nullptr;
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

bool ResultsView::isCurve()
{
    auto model = getModel_ptr();
    if (model != nullptr)
        return model->is_curve;
    else
        return false;
}


GraphViewResults::variable_t ResultsView::getMainVariable() const
{

    if (mCurrentVariableList.contains(GraphViewResults::eThetaEvent))
        return GraphViewResults::eThetaEvent;
#ifdef S02_BAYESIAN
    else if (mCurrentVariableList.contains(GraphViewResults::eS02))
        return GraphViewResults::eS02;
#endif

#ifdef KOMLAN
    else if (mCurrentVariableList.contains(GraphViewResults::eS02Vg))
        return GraphViewResults::eS02Vg;
#endif
    else if (mCurrentVariableList.contains(GraphViewResults::eSigma))
        return GraphViewResults::eSigma;

    else if (mCurrentVariableList.contains(GraphViewResults::eVg))
        return GraphViewResults::eVg;

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

    else
        return GraphViewResults::eThetaEvent;
}
