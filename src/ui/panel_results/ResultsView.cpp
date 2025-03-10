/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2024

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

#include "Bound.h"
#include "GraphView.h"
#include "GraphViewDate.h"
#include "GraphViewEvent.h"
#include "GraphViewLambda.h"
#include "GraphViewCurve.h"
#include "GraphViewPhase.h"
#include "GraphViewS02.h"

#include "ProjectView.h"
#include "QtCore/qobjectdefs.h"
#include "StdUtilities.h"
#include "Tabs.h"
#include "Ruler.h"
#include "Marker.h"

#include "Date.h"
#include "Event.h"
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

#include <QtWidgets>
#include <QtSvg>
#include <QFontDialog>


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
    mGraphTypeTabs->setFixedHeight(mGraphTypeTabs->tabHeight());

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
    int h = 15;

    // -----------------------------------------------------------------
    //  Results Group (if graph list tab = events or phases)
    // -----------------------------------------------------------------
    mEventsGroup = new QWidget(this);

    mEventsDatesUnfoldCheck = new CheckBox(tr("Unfold Data"));
    mEventsDatesUnfoldCheck->setFixedHeight(h);
    mEventsDatesUnfoldCheck->setToolTip(tr("Display Events' data"));

    mEventThetaRadio = new RadioButton(tr("Event Date"));
    mEventThetaRadio->setFixedHeight(h);
    mEventThetaRadio->setChecked(true);

    mDataSigmaRadio = new RadioButton(tr("Std ti"));
    mDataSigmaRadio->setFixedHeight(h);
#ifdef S02_BAYESIAN
    mS02Radio = new RadioButton(tr("Event Shrinkage"));
    mS02Radio->setFixedHeight(h);
#endif
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
#ifdef S02_BAYESIAN
    resultsGroupLayout->addWidget(mS02Radio);
#endif
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
    mPhasesEventsUnfoldCheck->setToolTip(tr("Display Phases' Events"));

    mPhasesDatesUnfoldCheck = new CheckBox(tr("Unfold Data"), mPhasesGroup);
    mPhasesDatesUnfoldCheck->setFixedHeight(h);
    mPhasesDatesUnfoldCheck->setToolTip(tr("Display Events' Data"));

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

    mCurveDataPointsCheck = new CheckBox(tr("Reference Points (HPD)"), mCurvesGroup);
    mCurveDataPointsCheck->setFixedHeight(h);
    mCurveDataPointsCheck->setChecked(true);

    mCurveEventsPointsCheck = new CheckBox(tr("Event Dates (HPD)"), mCurvesGroup);
    mCurveEventsPointsCheck->setFixedHeight(h);
    mCurveEventsPointsCheck->setChecked(false);


    mCurveGPRadio = new RadioButton(tr("Curve Var. Rate"), mCurvesGroup);
    mCurveGPRadio->setFixedHeight(h);
    
    mCurveGSRadio = new RadioButton(tr("Curve Acceleration"), mCurvesGroup);
    mCurveGSRadio->setFixedHeight(h);
    
    mLambdaRadio = new RadioButton(tr("Curve Smoothing"), mCurvesGroup);
    mLambdaRadio->setFixedHeight(h);
    
    mS02VgRadio = new RadioButton(tr("Curve Shrinkage"), mCurvesGroup);
    mS02VgRadio->setFixedHeight(h);

    mCurveStatCheck = new CheckBox(tr("Show Stat."));
    mCurveStatCheck->setFixedHeight(h);
    mCurveStatCheck->setToolTip(tr("Display numerical results computed on posterior densities below all graphs."));

    QVBoxLayout* curveGroupLayout = new QVBoxLayout();

    QVBoxLayout* curveOptionGroupLayout = new QVBoxLayout();
    curveOptionGroupLayout->setContentsMargins(15, 0, 0, 0);
    curveOptionGroupLayout->addWidget(mCurveErrorCheck, Qt::AlignLeft);
    curveOptionGroupLayout->addWidget(mCurveMapCheck, Qt::AlignLeft);
    curveOptionGroupLayout->addWidget(mCurveDataPointsCheck, Qt::AlignLeft);
    curveOptionGroupLayout->addWidget(mCurveEventsPointsCheck, Qt::AlignLeft);

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
#ifdef S02_BAYESIAN
    connect(mS02Radio, &RadioButton::clicked, this, &ResultsView::applyCurrentVariable);
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
    mGraphListTab = new Tabs(this);
    mGraphListTab->setFixedHeight(mGraphListTab->tabHeight());
    mGraphListTab->addTab( tr("Events"));
    mGraphListTab->addTab(tr("Phases"));
    mGraphListTab->addTab(tr("Curves"));

    connect(mGraphListTab, static_cast<void (Tabs::*)(const qsizetype&)>(&Tabs::tabClicked), this, &ResultsView::applyGraphListTab);

    // -----------------------------------------------------------------
    //  Tabs : Display / Distrib. Options
    // -----------------------------------------------------------------
    mDisplayDistribTab = new Tabs(this);
    mDisplayDistribTab->setFixedHeight(mDisplayDistribTab->tabHeight());

    mDisplayDistribTab->addTab(tr("Display"));
    mDisplayDistribTab->addTab(tr("Distrib. Options"));

    // Necessary to reposition all elements inside the selected tab :
    connect(mDisplayDistribTab, static_cast<void (Tabs::*)(const qsizetype&)>(&Tabs::tabClicked), this, &ResultsView::updateOptionsWidget);

    // -----------------------------------------------------------------
    //  Display / Span Options
    // -----------------------------------------------------------------
    mDisplayWidget = new QWidget(this);
    mSpanGroup  = new QWidget(this);
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

    mTimeLab = new QLabel(tr("Time"), mSpanGroup);
    mTimeLab->setFixedHeight(h);
    mTimeLab->setAlignment(Qt::AlignCenter);

    mTimeSlider = new QSlider(Qt::Horizontal, mSpanGroup);
    mTimeSlider->setFixedHeight(h);
    mTimeSlider->setRange(-100, 100);
    mTimeSlider->setTickInterval(1);
    mTimeSlider->setValue(0);

    mTimeEdit = new LineEdit(mSpanGroup);
    mTimeEdit->setValidator(RplusValidator);
    mTimeEdit->setFixedHeight(h);
    mTimeEdit->setText(locale().toString(sliderToZoom(mTimeSlider->value())));
    mTimeEdit->setToolTip(tr("Enter zoom value to magnify the curves on X span"));
    mTimeEdit->setFixedWidth(mOptionsW/3); //for windows new spin box

    /*QString decimalSeparator = locale().decimalPoint();
    QString inputMask = "000" + decimalSeparator + "00%";

    mTimeEdit->setInputMask(inputMask);
    */


    mMajorScaleLab = new QLabel(tr("Major Interval"), mSpanGroup);
    mMajorScaleLab->setFixedHeight(h);
    mMajorScaleLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mMajorScaleEdit = new LineEdit(mSpanGroup);
    mMajorScaleEdit->setFixedHeight(h);
    mMajorScaleEdit->setText(QString::number(mMajorScale));
    mMajorScaleEdit->setToolTip(tr("Enter an interval for the main axis division below the curves, greater than 1"));

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
    h = 20;

    mXOptionTitle = new Label(tr("X Scale"), mDisplayWidget);
    mXOptionTitle->setFixedHeight(25);
    mXOptionTitle->setIsTitle(true);

    mXOptionBut = new Button(tr("Optimal X Display"), mXOptionGroup);
    mXOptionBut->setFixedHeight(25);
    mXOptionBut->setToolTip(tr("Restore view with the study period span"));

    mXOptionLab = new QLabel(tr("X"), mXOptionGroup);
    mXOptionLab->setFixedHeight(h);

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

    mZoomEdit = new LineEdit(mGraphicGroup);
    mZoomEdit->setValidator(RplusValidator);
    mZoomEdit->setToolTip(tr("Enter zoom value to increase graph height"));
    mZoomEdit->setFixedWidth(mOptionsW/3); //for windows new spin box
    mZoomEdit->setText(locale().toString(mZoomSlider->value()));



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
    mCredibilityCheck->setFixedHeight(h);
    mCredibilityCheck->setChecked(true);

    mThreshLab = new QLabel(tr("Confidence Level (%)"), mDensityOptsGroup);
    mThreshLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mThresholdEdit = new LineEdit(mDensityOptsGroup);
    DoubleValidator* percentValidator = new DoubleValidator();
    percentValidator->setBottom(0.0);
    percentValidator->setTop(100.0);
    mThresholdEdit->setValidator(percentValidator);
    mThresholdEdit->setFixedHeight(16);

    // Used with Activity
    mRangeThreshLab = new QLabel(tr("Time Range Level (%)"), mDensityOptsGroup);
    mRangeThreshLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mRangeThresholdEdit = new LineEdit(mDensityOptsGroup);
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

    mBandwidthEdit = new LineEdit(mDensityOptsGroup);
    mBandwidthEdit->setValidator(RplusValidator);
    mBandwidthEdit->setFixedHeight(20);

    mHActivityLab = new QLabel(tr("Activity Bandwidth"), mDensityOptsGroup);
    mHActivityEdit = new LineEdit(mDensityOptsGroup);
    mHActivityEdit->setValidator(RplusValidator);
    mHActivityEdit->setFixedHeight(16);
    
    connect(mCredibilityCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mFFTLenCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ResultsView::applyFFTLength);
    connect(mBandwidthEdit, &LineEdit::editingFinished, this, &ResultsView::applyBandwidth);
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
    densityLayout5->addWidget(mBandwidthEdit);

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

    mPageEdit = new LineEdit(mPageWidget);
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
    mCurvesScrollArea->setVisible(false);

    GraphViewResults::mHeightForVisibleAxis = 4 * AppSettings::heigthUnit(); // Only for init, it' s modify in updateGraphsLayout()
    mGraphHeight = GraphViewResults::mHeightForVisibleAxis; // Only for init

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
    mZooms.clear();
    mZoomsX.clear();
    mZoomsY.clear();
    mZoomsZ.clear();
    mCurrentVariableList.clear();
    mScales.clear();

}

void ResultsView::updateModel()
{
    createGraphs();
    updateLayout();

}

void ResultsView::initModel()
{
    auto model = getModel_ptr();


    mHasPhases = (model->mPhases.size() > 0);

    // ----------------------------------------------------
    //  Create Chains option controls (radio and checkboxes under "MCMC Chains")
    // ----------------------------------------------------
    createChainsControls();
    mAllChainsCheck->setChecked(true);

    mCurrentTypeGraph = GraphViewResults::ePostDistrib;
    mCurrentVariableList.clear();

    mRangeThresholdEdit->setText(stringForLocal(95.));
    mThresholdEdit->setText(stringForLocal(model->getThreshold()));
    mHActivityEdit->setText(stringForLocal(model->mHActivity));

    mFFTLenCombo->setCurrentText(stringForLocal(model->getFFTLength()));
    mBandwidthEdit->blockSignals(true);
    mBandwidthEdit->setText(locale().toString((model->getBandwidth())));
    mBandwidthEdit->blockSignals(false);

    mZooms.clear();
    mZoomsX.clear();
    mZoomsY.clear();
    mZoomsZ.clear();

    updateGraphsMinMax();
    applyStudyPeriod();

    if (isCurve()) {
        mMainVariable = GraphViewResults::eG;
        mCurveGRadio->setChecked(true);
        mGraphListTab->setTab(2, false);

        const auto &gx = model->mPosteriorMeanG.gx;
        const auto minmax_Y = gx.mapG.rangeY;

        double minY = +INFINITY;
        double maxY = -INFINITY;
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
            const auto minmax_Y = gy.mapG.rangeY;

            minY = +INFINITY;
            maxY = -INFINITY;
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
                const auto minmax_Y = gz.mapG.rangeY;

                minY = +INFINITY;
                maxY = -INFINITY;
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
        mGraphListTab->setTab(1, false);

    } else {
        mMainVariable = GraphViewResults::eThetaEvent;
        mGraphListTab->setTab(0, false);
    }
    updateMainVariable();
    mCurrentVariableList.append(mMainVariable);

    updateOptionsWidget();
    createGraphs();
    updateLayout(); // done in showStats() ??

    showStats(mStatCheck->isChecked());
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
    if (mStatCheck->isChecked() || mPhasesStatCheck->isChecked() || mCurveStatCheck->isChecked()) {
        graphWidth = (2./3.) * leftWidth;
    }

    mGraphTypeTabs->setGeometry(mMargin, mMargin, leftWidth, tabsH);
    mRuler->setGeometry(mMargin, mMargin + tabsH, graphWidth, Ruler::sHeight);

    const int stackH = height() - mMargin - tabsH - Ruler::sHeight;
    const QRect graphScrollGeometry(mMargin, mMargin + tabsH + Ruler::sHeight, leftWidth, stackH);

    mEventsScrollArea->setGeometry(graphScrollGeometry);
    mPhasesScrollArea->setGeometry(graphScrollGeometry);
    mCurvesScrollArea->setGeometry(graphScrollGeometry);

    if (mGraphListTab->currentIndex() == 2 )
        GraphViewResults::mHeightForVisibleAxis = 20 * AppSettings::heigthUnit() / mByCurvesGraphs.size();
    else
        GraphViewResults::mHeightForVisibleAxis = 4 * AppSettings::heigthUnit() ;
    mGraphHeight = GraphViewResults::mHeightForVisibleAxis;

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
        }else if (mEventVGRadio->isChecked()) {
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

        } else if (mCurveGPRadio->isChecked()) {
            mMainVariable = GraphViewResults::eGP;

            if (mCurveMapCheck->isChecked())
                mCurrentVariableList.append(GraphViewResults::eMap);


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
                                              + mBandwidthEdit->height() + mHActivityEdit->height() + mRangeThresholdEdit->height()
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
    auto model =getModel_ptr();
    if (model->mChains.size() != (size_t)mChainChecks.size()) {
        deleteChainsControls();

        for (size_t i=0; i<model->mChains.size(); ++i) {
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

    } else  if (mS02VgRadio->isChecked())  {
        GraphViewS02* graphS02 = new GraphViewS02(mCurvesWidget);
        setGraphicOption(*graphS02);

        graphS02->setTitle(tr("Curve Shrinkage"));

        mByCurvesGraphs.append(graphS02);

        connect(graphS02, &GraphViewResults::selected, this, &ResultsView::updateOptionsWidget);

    } else  {
        const bool displayY = model->displayY();
        const bool displayZ = model->displayZ();

        // insert refpoints for X
        //  const double thresh = 68.4;

        double pt_Ymin(0.), pt_Ymax(0.);
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
                                for (const auto& h : intervals) {
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
                            dataPts[iDataPts].Ymin = event->mZField - 1.96*event->mS_ZField;
                            dataPts[iDataPts].Ymax = event->mZField + 1.96*event->mS_ZField;
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

void ResultsView::deleteAllGraphsInList(QList<GraphViewResults*> &list)
{
    for (auto&& graph : list) {
        disconnect(graph, nullptr, nullptr, nullptr); //Disconnect everything connected to
        delete graph;
        graph = nullptr;
    }
    list.clear();
    list.shrink_to_fit();
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

    updateCurvesToShow();

}

void ResultsView::updateGraphsMinMax()
{
    auto model = getModel_ptr();
    QList<GraphViewResults*> listGraphs = currentGraphs(false);
    if (mCurrentTypeGraph == GraphViewResults::ePostDistrib) {

        if (mMainVariable == GraphViewResults::eDuration ||
            mMainVariable == GraphViewResults::eS02 ||
            mMainVariable == GraphViewResults::eS02Vg
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
    if ((mGraphListTab->currentName() == tr("Curves")) && !mLambdaRadio->isChecked() && !mS02VgRadio->isChecked()) {

        if (mCurveGRadio->isChecked()) {
            showVariableList.append(GraphViewResults::eG);
            if (mCurveErrorCheck->isChecked()) showVariableList.append(GraphViewResults::eGError);
            if (mCurveMapCheck->isChecked()) showVariableList.append(GraphViewResults::eMap);
            if (mCurveEventsPointsCheck->isChecked()) showVariableList.append(GraphViewResults::eGEventsPts);
            if (mCurveDataPointsCheck->isChecked()) showVariableList.append(GraphViewResults::eGDatesPts);
        }
        if (mCurveGPRadio->isChecked()) {
            showVariableList.append(GraphViewResults::eGP);
            if (mCurveMapCheck->isChecked()) showVariableList.append(GraphViewResults::eMap);

        }

       if (mCurveGSRadio->isChecked()) showVariableList.append(GraphViewResults::eGS);

       const bool showStat = mCurveStatCheck->isChecked();

        // --------------------------------------------------------
        //  Update Graphs with selected options
        // --------------------------------------------------------
        for (GraphViewResults*& graph : listGraphs) {

            GraphViewCurve* graphCurve = static_cast<GraphViewCurve*>(graph);
            graphCurve->setShowNumericalResults(showStat);

            QString graphName = model->getCurvesLongName().at(0);
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
            Scale t_scale;
            /*t_scale.findOptimalMark(mResultCurrentMinT, mResultCurrentMaxT, 10);
            mMajorScale = t_scale.mark;
            mMinorCountScale = 10;
            */
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
         //mTimeSpin->setRange(sliderToZoom(-100), sliderToZoom(100));
         //mTimeSpin->setSingleStep(.01);
         //mTimeSpin->setDecimals(3);

         // The Ruler range is much wider based on the minimal zoom
         const double tCenter = (mResultCurrentMinT + mResultCurrentMaxT) / 2.;
         const double tSpan = mResultCurrentMaxT - mResultCurrentMinT;
         const double tRangeMin = tCenter - ((tSpan/2.) / sliderToZoom(mTimeSlider->minimum()));
         const double tRangeMax = tCenter + ((tSpan/2.) / sliderToZoom(mTimeSlider->minimum()));

         mRuler->setRange(tRangeMin, tRangeMax);
         mRuler->setFormatFunctX(nullptr);

    } else if (mCurrentTypeGraph == GraphViewResults::ePostDistrib &&
               ( mMainVariable == GraphViewResults::eSigma ||
                 mMainVariable == GraphViewResults::eS02 ||
                 mMainVariable == GraphViewResults::eVg ||
                 mMainVariable == GraphViewResults::eDuration ||
                 mMainVariable == GraphViewResults::eS02Vg ) ) {

                // The X zoom uses a log scale on the spin box and can be controlled by the linear slider
                mTimeSlider->setRange(-100, 100);
                //mTimeSpin->setRange(sliderToZoom(-100), sliderToZoom(100));
                //mTimeSpin->setSingleStep(.01);
                //mTimeSpin->setDecimals(3);

                // The Ruler range is much wider based on the minimal zoom
                const double tRangeMax = mResultMaxT / sliderToZoom(mTimeSlider->minimum());

                mRuler->setRange(0, tRangeMax);
                mRuler->setFormatFunctX(nullptr);



    } else if (mCurrentTypeGraph == GraphViewResults::ePostDistrib &&
               mMainVariable == GraphViewResults::eLambda ) {

                // The X zoom uses a log scale on the spin box and can be controlled by the linear slider
                mTimeSlider->setRange(-100, 100);
                //mTimeSpin->setRange(sliderToZoom(-100), sliderToZoom(100));
                //mTimeSpin->setSingleStep(.01);
                //mTimeSpin->setDecimals(3);

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
        for (const auto& chRadio : mChainRadios) {
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
    setTimeEdit(mResultZoomT);
    setTimeScale();

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
            updateCurvesToShow();

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
            updateCurvesToShow();

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
                updateCurvesToShow();

            } else {
                findOptimalZ();
            }
        }
    }

}

void ResultsView::createOptionsWidget()
{

    unsigned optionWidgetHeigth = 0;
    // -------------------------------------------------------------------------------------
    //  Update graph list tab
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

   // }
/*    else if (!mHasPhases && !isCurve() && mGraphListTab->currentIndex() >= 1) {
        mGraphListTab->setTab(0, false);
    }
    */

    optionWidgetHeigth += mGraphListTab->height();
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
#else
        qreal totalH =  3*h;
#endif

        mEventVGRadio->hide();


        eventGroupLayout->addWidget(mEventsDatesUnfoldCheck);


        mDataCalibCheck->hide();
        mWiggleCheck->hide();

        eventGroupLayout->addWidget(mStatCheck);
        totalH += h;

        delete mEventsGroup->layout() ;
        mEventsGroup->setLayout(eventGroupLayout);

        //-- end new layout

        mEventsGroup->setFixedHeight(totalH);

        optionWidgetHeigth += mEventsGroup->height();



    // ------------------------------------
    //  Display / Distrib. Option
    // ------------------------------------
    optionWidgetHeigth += mDisplayDistribTab->height();

    mOptionsLayout->addWidget(mDisplayDistribTab);

    qreal widHeigth = 0;
    const  qreal internSpan = 10;
    if (true) {//mDisplayDistribTab->currentName() == tr("Display") ) {
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


            mXOptionTitle->setVisible(false);
            mXOptionGroup->setVisible(false);

            mYOptionTitle->setVisible(false);
            mYOptionGroup->setVisible(false);

            mZOptionTitle->setVisible(false);
            mZOptionGroup->setVisible(false);
            mDisplayWidget-> setFixedHeight(widHeigth);


        optionWidgetHeigth += widHeigth;

    }
    optionWidgetHeigth += 35;//40; // ???

    // -------------------------------------------------------------------------------------
    //  Page / Save
    // -------------------------------------------------------------------------------------
    mOptionsLayout->addWidget(mPageSaveTab);
    optionWidgetHeigth += mPageSaveTab->height();

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

        mPageEdit->setText(locale().toString(mCurrentPage + 1) + "/" + locale().toString(numPages));

        mPageWidget->setVisible(true);
        mSaveSelectWidget->hide();
        mSaveAllWidget->hide();

        mOptionsLayout->addWidget(mPageWidget);
        optionWidgetHeigth += mPageWidget->height();

    }

    mOptionsLayout->addStretch();
    mOptionsWidget->setLayout(mOptionsLayout);
    mOptionsWidget->setGeometry(0, 0, mOptionsW - mMargin, optionWidgetHeigth);
}


void ResultsView::updateOptionsWidget()
{
    auto project = getProject_ptr();
    if (project ==  nullptr)
        return;

    auto model = getModel_ptr();
    if (model ==  nullptr)
        return;

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
#ifdef S02_BAYESIAN
        eventGroupLayout->addWidget(mS02Radio);
        qreal totalH =  4*h;
#else
        qreal totalH =  3*h;
#endif
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

        mErrCheck->setText("Error");

        if (mBeginEndRadio->isChecked() || mDurationRadio->isChecked()) {

           mErrCheck->hide();
           mActivityUnifCheck->hide();

        } else if (mTempoRadio->isChecked()) {

            mErrCheck->show();
            mErrCheck->setText("Error Clopper-Pearson");
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

        mCurveErrorCheck->hide();
        mCurveMapCheck->hide();
        mCurveEventsPointsCheck->hide();
        mCurveDataPointsCheck->hide();

        if (mCurveGRadio->isChecked()) {
            mCurveErrorCheck->show();
            mCurveMapCheck->show();
            mCurveEventsPointsCheck->show();
            mCurveDataPointsCheck->show();
            totalH += 4 * mCurveErrorCheck->height()*1.3 + 5. ;

            QVBoxLayout* curveOptionGroupLayout = new QVBoxLayout();
            curveOptionGroupLayout->setContentsMargins(15, 0, 0, 0);
            curveOptionGroupLayout->addWidget(mCurveErrorCheck, Qt::AlignLeft);
            curveOptionGroupLayout->addWidget(mCurveMapCheck, Qt:: AlignLeft);
            curveOptionGroupLayout->addWidget(mCurveDataPointsCheck, Qt::AlignLeft);
            curveOptionGroupLayout->addWidget(mCurveEventsPointsCheck, Qt::AlignLeft);

            curveGroupLayout->addLayout(curveOptionGroupLayout);

        }
        curveGroupLayout->addWidget(mCurveGPRadio);
        if (mCurveGPRadio->isChecked()) {
            mCurveMapCheck->show();
            totalH += mCurveMapCheck->height()*1.3 + 5.;

            QVBoxLayout* curveOptionGroupLayout = new QVBoxLayout();
            curveOptionGroupLayout->setContentsMargins(15, 0, 0, 0);
            curveOptionGroupLayout->addWidget(mCurveMapCheck, Qt:: AlignLeft);

            curveGroupLayout->addLayout(curveOptionGroupLayout);
        }

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

        const bool displayX = model ? model->displayX() : false;
        const bool displayY = model ? model->displayY() : false;
        const bool displayZ = model ? model->displayZ() : false;

        if (isCurve() && ( mMainVariable == GraphViewResults::eG ||
                           mMainVariable == GraphViewResults::eGP ||
                           mMainVariable == GraphViewResults::eGS)) {
            mXOptionTitle->setVisible(displayX);
            mXOptionGroup->setVisible(displayX);
            widHeigth += displayX ? 2*h + 2*internSpan : 0.;

            mYOptionTitle->setVisible(displayY);
            mYOptionGroup->setVisible(displayY);
            widHeigth += displayY ? 2*h + 2*internSpan : 0.;

            mZOptionTitle->setVisible(displayZ);
            mZOptionGroup->setVisible(displayZ);
            widHeigth += displayZ ? 2*h + 3*internSpan : 0.;
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

        optionWidgetHeigth += widHeigth;

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

        mCredibilityCheck->show();
        mCredibilityCheck->setFixedHeight(15);

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


            mBandwidthLab->hide();
            mBandwidthLab->setFixedHeight(0);
            mBandwidthEdit->hide();
            mBandwidthEdit->setFixedHeight(0);

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

            mBandwidthLab->hide();
            mBandwidthLab->setFixedHeight(0);
            mBandwidthEdit->hide();
            mBandwidthEdit->setFixedHeight(0);

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

            mBandwidthLab->show();
            mBandwidthLab->setFixedHeight(20);
            mBandwidthEdit->show();
            mBandwidthEdit->setFixedHeight(20);

            mHActivityLab->hide();
            mHActivityLab->setFixedHeight(0);
            mHActivityEdit->hide();
            mHActivityEdit->setFixedHeight(0);

            nbObject += 2;
        }

        mDensityOptsTitle->setVisible(showDensityOptions);
        mDensityOptsGroup->setVisible(showDensityOptions);

        if (showDensityOptions) {

            mDensityOptsGroup->setFixedHeight( mCredibilityCheck->height() + mThresholdEdit->height() + mFFTLenCombo->height()
                                               + mBandwidthEdit->height() + mHActivityEdit->height() + mRangeThresholdEdit->height()
                                               + (nbObject+2)* internSpan);

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
    double origin = GraphViewResults::mHeightForVisibleAxis;
    //if (mGraphListTab->currentIndex() == 2 )
     //   origin = 0.75 * height() / mByCurvesGraphs.size();

    const double prop = locale().toDouble(mZoomEdit->text()) / 100.;
    mGraphHeight = min + prop * (origin - min);
    updateGraphsLayout();
}

void ResultsView::updateZoomT()
{
    // Pick the value from th spin or the slider
    const double zoom = locale().toDouble(mTimeEdit->text())/100.;

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

    mCurrentTMinEdit->setText(locale.toString(mResultCurrentMinT));
    mCurrentTMaxEdit->setText(locale.toString(mResultCurrentMaxT));

    mCurrentTMinEdit->blockSignals(false);
    mCurrentTMaxEdit->blockSignals(false);
}

void ResultsView::setTimeEdit(const double value)
{
    mTimeEdit->blockSignals(true);
    mTimeEdit->setText(locale().toString(value * 100.));
    mTimeEdit->blockSignals(false);
}

void ResultsView::setTimeSlider(const int value)
{
    mTimeSlider->blockSignals(true);
    mTimeSlider->setValue(value);
    mTimeSlider->blockSignals(false);
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
    auto model = getModel_ptr();
    if (xScaleRepresentsTime()) {
      mResultCurrentMinT = model->mSettings.getTminFormated();
      mResultCurrentMaxT = model->mSettings.getTmaxFormated();

    } else if ( mMainVariable == GraphViewResults::eSigma ||
                mMainVariable == GraphViewResults::eDuration ||
                mMainVariable == GraphViewResults::eS02  ||
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
        setTimeSlider(zoomToSlider(locale().toDouble(mTimeEdit->text())/100.));
        updateZoomT();
    }

}

# pragma mark Curve Zoom
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
    const auto model = getModel_ptr();
    const std::vector<double>* vec = nullptr;

    Scale XScale;
    if (mCurveGRadio->isChecked()) {
        const auto minmax_Y = model->mPosteriorMeanG.gx.mapG.rangeY;
        vec = &model->mPosteriorMeanG.gx.vecG;
        const std::vector<double>* vecVar = &model->mPosteriorMeanG.gx.vecVarG;

        double minY = +INFINITY;
        double maxY = -INFINITY;
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
            const auto minmax_Y = model->mPosteriorMeanG.gx.mapGP.rangeY;
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

        double minY = +INFINITY;
        double maxY = -INFINITY;
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
            const auto minmax_Y = model->mPosteriorMeanG.gy.mapGP.rangeY;
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

        double minY = +INFINITY;
        double maxY = -INFINITY;
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
            const auto minmax_Y = model->mPosteriorMeanG.gz.mapGP.rangeY;
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
    if (!isMajorNumber)
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
    mZoomEdit->blockSignals(true);
    mZoomEdit->setText(locale().toString(value));
    mZoomEdit->blockSignals(false);

    updateGraphsHeight();
}

void ResultsView::applyZoomEdit()
{
    if (mZoomEdit->hasAcceptableInput()) {
        mZoomSlider->blockSignals(true);
        mZoomSlider->setValue(locale().toInt(mZoomEdit->text()));
        mZoomSlider->blockSignals(false);

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
        }

        mFontBut->setText(font.family() + ", " + font.styleName() + ", " +QString::number(font.pointSizeF()));
        mFontBut->setFont(font);
    }
}

void ResultsView::applyThickness(int value)
{
    const QList<GraphViewResults*>& graphs = allGraphs();
    for (const auto& graph : graphs) {
        graph->setGraphsThickness(value);
    }
}

void ResultsView::applyOpacity(int value)
{
    const int opValue = value * 10;
    const QList<GraphViewResults*> &graphs = allGraphs();
    for (const auto& graph : graphs) {
        graph->setGraphsOpacity(opValue);
    }
}


void ResultsView::setGraphicOption(GraphViewResults& graph)
{
    auto model = getModel_ptr();
    graph.setSettings(model->mSettings);
    graph.setMCMCSettings(model->mMCMCSettings, model->mChains);
    graph.setGraphsFont(mFontBut->font());
    graph.setGraphsThickness(mThicknessCombo->currentIndex());
    graph.changeXScaleDivision(mMajorScale, mMinorCountScale);
    graph.setGraphsOpacity(mOpacityCombo->currentIndex()*10);
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
        const double h = locale().toDouble(mHActivityEdit->text());
        const double rangePercent = locale().toDouble(mRangeThresholdEdit->text());
        getModel_ptr()->setHActivity(h, rangePercent);
        generateCurves();
    }

}

void ResultsView::applyBandwidth()
{
   if (mBandwidthEdit->hasAcceptableInput()) {
        const double bandwidth = locale().toDouble(mBandwidthEdit->text());
        getModel_ptr()->setBandwidth(bandwidth);
        generateCurves();
    }
}

void ResultsView::applyThreshold()
{
    const double hpd = locale().toDouble(mThresholdEdit->text());
    getModel_ptr()->setThreshold(hpd);
    generateCurves();
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

                file.setFileName(dirPath + "/Curve_" +list_names.at(0) + "_Map.csv");

                if (file.open(QFile::WriteOnly | QFile::Truncate)) {
                    model->saveMapToFile(&file, csvSep, model->mPosteriorMeanG.gx.mapG);

                }

                if (model->displayY()) {
                    file.setFileName(dirPath + "/Curve"+list_names.at(1) + "_Map.csv");

                    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
                        model->saveMapToFile(&file, csvSep, model->mPosteriorMeanG.gy.mapG);
                        file.close();
                    }

                    if (model->displayZ()) {
                        file.setFileName(dirPath + "/Curve"+list_names.at(2) + "_Map.csv");

                        if (file.open(QFile::WriteOnly | QFile::Truncate)) {
                            model->saveMapToFile(&file, csvSep, model->mPosteriorMeanG.gz.mapG);
                            file.close();
                        }
                    }

                }

                // --------------   Saving Curve Ref
                int i = 0;
                for (auto&& graph : mByCurvesGraphs) {
                    graph->getGraph()->exportReferenceCurves ("", QLocale::English, ",",  model->mSettings.mStep, dirPath + "/Curve_"+list_names.at(i) + "_ref.csv" );
                    i++;
                }

                // Second Map GP

                file.setFileName(dirPath + "/Curve_" +list_names.at(0) + "_MapGP.csv");

                if (file.open(QFile::WriteOnly | QFile::Truncate)) {
                    model->saveMapToFile(&file, csvSep, model->mPosteriorMeanG.gx.mapGP);

                }

                if (model->displayY()) {
                    file.setFileName(dirPath + "/Curve"+list_names.at(1) + "_MapGP.csv");

                    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
                        model->saveMapToFile(&file, csvSep, model->mPosteriorMeanG.gy.mapGP);
                        file.close();
                    }

                    if (model->displayZ()) {
                        file.setFileName(dirPath + "/Curve"+list_names.at(2) + "_MapGP.csv");

                        if (file.open(QFile::WriteOnly | QFile::Truncate)) {
                            model->saveMapToFile(&file, csvSep, model->mPosteriorMeanG.gz.mapGP);
                            file.close();
                        }
                    }

                }

                // --------------   Saving Curve Ref
                i = 0;
                for (auto&& graph : mByCurvesGraphs) {
                    graph->getGraph()->exportReferenceCurves ("", QLocale::English, ",",  model->mSettings.mStep, dirPath + "/Curve_"+list_names.at(i) + "_ref.csv" );
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
    else if (mCurrentVariableList.contains(GraphViewResults::eS02))
        return GraphViewResults::eS02;

    else if (mCurrentVariableList.contains(GraphViewResults::eSigma))
        return GraphViewResults::eSigma;
    else if (mCurrentVariableList.contains(GraphViewResults::eVg))
        return GraphViewResults::eVg;
    //else if (mCurrentVariableList.contains(GraphViewResults::eSigma))
    //    return GraphViewResults::eSigma;

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
