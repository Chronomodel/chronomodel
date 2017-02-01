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

#include "AppSettings.h"

#include <QtWidgets>
#include <iostream>
#include <QtSvg>

#pragma mark Constructor & Destructor
ResultsView::ResultsView(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mResultMaxVariance(1000.),
mHasPhases(false),
mModel(nullptr),
mMargin(5),
mOptionsW(200),
mLineH(15),
mGraphLeft(130),
mRulerH(40),
mTabsH(30),
mGraphsH(130),
mTabEventsIndex(0),
mTabPhasesIndex(0),
mEventsScrollArea(nullptr),
mPhasesScrollArea(nullptr),
mCurrentTypeGraph(GraphViewResults::ePostDistrib), //mTabs=0
mBandwidthUsed(1.06),
mThresholdUsed(95.0),
mNumberOfGraph(APP_SETTINGS_DEFAULT_SHEET)
{
    mResultMinX = mSettings.mTmin;
    mResultMaxX = mSettings.mTmax;
    mResultCurrentMinX = mResultMinX ;
    mResultCurrentMaxX = mResultMaxX ;
    mResultZoomX = 1.;
    
    QFont fontTitle (QApplication::font());
    //fontTitle.setPointSizeF(QApplication::font().pointSizeF()*1.);

    QFont font(QApplication::font());
    QFontMetricsF fm(font);

    mTabs = new Tabs(this);
    mTabs->addTab(tr("Posterior distrib."));
    mTabs->addTab(tr("History plots"));
    mTabs->addTab(tr("Acceptation rate"));
    mTabs->addTab(tr("Autocorrelation"));
    mTabs->setTab(0, false);
    mTabs->setFont(fontTitle);
    // -------------
    
    mStack = new QStackedWidget(this);
    
    // Make when we need it, within createEventsScrollArea and within createPhasesScrollArea
 /*   mEventsScrollArea = new QScrollArea();
    mEventsScrollArea->setMouseTracking(true);
    mStack->addWidget(mEventsScrollArea);
    mEventsWidget = new QWidget(mEventsScrollArea);
    mEventsWidget->setMouseTracking(true);
  */
   /* mPhasesScrollArea = new QScrollArea();
    mPhasesScrollArea->setMouseTracking(true);
    mStack->addWidget(mPhasesScrollArea);*/
    
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
    mUnfoldBut->setFixedHeight(int(mRulerH/2));
    mUnfoldBut->setCheckable(true);
    mUnfoldBut->setFlatHorizontal();
    mUnfoldBut->setIcon(QIcon(":unfold.png"));
    mUnfoldBut->setToolTip(tr("Display event's data or phase's events, depending on the chosen layout."));
    
    mNextSheetBut  = new Button(tr("Next"), this);
    mNextSheetBut->setFixedHeight(int(mRulerH/2));
    mNextSheetBut->setCheckable(false);
    mNextSheetBut->setFlatHorizontal();
   // mNextSheetBut->setIcon(QIcon(":unfold.png"));
    mNextSheetBut->setToolTip(tr("Display other data"));

    mPreviousSheetBut  = new Button(tr("Prev."), this);
    mPreviousSheetBut->setFixedHeight(int(mRulerH/2));
    mPreviousSheetBut->setCheckable(false);
    mPreviousSheetBut->setFlatHorizontal();
   // mPreviousSheetBut->setIcon(QIcon(":unfold.png"));
    mPreviousSheetBut->setToolTip(tr("Display other data"));


    mStatsBut = new Button(tr("Stats"));
    mStatsBut->setCheckable(true);
    mStatsBut->setFlatHorizontal();
    mStatsBut->setIcon(QIcon(":stats_w.png"));
    mStatsBut->setFixedHeight(50);
    mStatsBut->setToolTip(tr("Display numerical results computed on posterior densities below all graphs."));
    
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
    displayButsLayout->addWidget(mStatsBut);
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
    mDisplayGroup->setFont(font);
    
    mDisplayTitle = new Label(tr("Display Options"));
    mDisplayTitle->setIsTitle(true);
    
    mCurrentXMinEdit = new LineEdit(mDisplayGroup);
    mCurrentXMinEdit->setFont(font);
    mCurrentXMinEdit->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");
    mCurrentXMinEdit->setAlignment(Qt::AlignHCenter);   
    mCurrentXMinEdit->setFixedSize(60, fm.height()+2);

    mCurrentXMaxEdit = new LineEdit(mDisplayGroup);
    mCurrentXMaxEdit->setFont(font);
    mCurrentXMaxEdit->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");
    mCurrentXMaxEdit->setAlignment(Qt::AlignHCenter);
    mCurrentXMaxEdit->setFixedSize(60, fm.height()+2);
    
    
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
    
    mFont.setPointSize(QApplication::font().pointSize());
    mFontBut = new QPushButton(mFont.family());
    connect(mFontBut, &QPushButton::clicked, this, &ResultsView::updateFont);
    
    mThicknessSpin = new QSpinBox();
    mThicknessSpin->setRange(1, 10);
    mThicknessSpin->setSuffix(" px");
   // mThicknessSpin->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }"); // not supported
    connect(mThicknessSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ResultsView::updateThickness);
    
    mOpacitySpin = new QSpinBox();
    mOpacitySpin->setRange(0, 100);
    mOpacitySpin->setValue(30);
    mOpacitySpin->setSuffix(" %");
    //mOpacitySpin->setStyleSheet("QLineEdit { border-radius: 5px; }"); // not supported

    connect(mOpacitySpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ResultsView::updateOpacity);


    // Grid : 8 columns
    QGridLayout* displayLayout = new QGridLayout();
    displayLayout->setContentsMargins(0, 0, 0, 0);
    displayLayout->setVerticalSpacing(6);
    displayLayout->setHorizontalSpacing(2);
    displayLayout->addWidget(mCurrentXMinEdit, 0, 0, 1, 2);
    displayLayout->addWidget(mCurrentXMaxEdit, 0, 6, 1, 2);
    displayLayout->addWidget(mXScaleLab, 0, 2, 1, 4);
    displayLayout->addWidget(mXSlider, 1, 0, 1, 8);
    displayLayout->addWidget(mYScaleLab, 2, 0, 1, 8);
    displayLayout->addWidget(mYSlider, 3, 0, 1, 8);
    
    labFont = new Label(tr("Font"));
    labThickness = new Label(tr("Thickness"));
    labOpacity = new Label(tr("Fill Opacity"));
    labRendering = new Label(tr("Rendering"));
    
    QFormLayout* displayForm = new QFormLayout();

    displayForm->setContentsMargins(0, 0, 0, 0);
    displayForm->addRow(labFont, mFontBut);
    displayForm->addRow(labThickness, mThicknessSpin);
    displayForm->addRow(labOpacity, mOpacitySpin);
    displayForm->addRow(labRendering, mRenderCombo);
    // Spacing must be done after addRow
    displayForm->setHorizontalSpacing(10);
    displayForm->setVerticalSpacing(0);

    //-------

    QVBoxLayout* displayLayoutWrapper = new QVBoxLayout();
    displayLayoutWrapper->setContentsMargins(6, 6, 6, 6);
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
    mDataCalibCheck = new CheckBox(tr("Individual calib. dates"), mResultsGroup);

    mWiggleCheck = new CheckBox(tr("Wiggle shifted"), mResultsGroup);
    mDataThetaRadio->setChecked(true);
    mDataCalibCheck->setChecked(true);
    mShowDataUnderPhasesCheck->setChecked(false);
    mShowDataUnderPhasesCheck->setVisible(false);
   
    /* -------------------------------------- mPostDistGroup ---------------------------------------------------*/
    
    mPostDistOptsTitle = new Label(tr("Post. distrib. options"));
    mPostDistOptsTitle->setIsTitle(true);
    mPostDistGroup = new QWidget();
    
    mCredibilityCheck = new CheckBox(tr("Show credibility"), mPostDistGroup);
    mCredibilityCheck->setChecked(true);
    mThreshLab = new Label(tr("HPD / Credibility (%)"), mPostDistGroup);
    mThreshLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    
    mHPDEdit = new LineEdit(mPostDistGroup);
    mHPDEdit->setFont(font);
    mHPDEdit->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");
    mHPDEdit->setText("95");
    
    DoubleValidator* percentValidator = new DoubleValidator();
    percentValidator->setBottom(0.0);
    percentValidator->setTop(100.0);
    percentValidator->setDecimals(1);
    mHPDEdit->setValidator(percentValidator);
    
    mFFTLenLab = new Label(tr("Grid length"), mPostDistGroup);
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
    
    mComboH = fm.height()+6;  //mFFTLenCombo->sizeHint().height();
    mTabsH = mComboH + 2*mMargin;
    
    mBandwidthLab = new Label(tr("Bandwidth const."), mPostDistGroup);
    mBandwidthLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    mBandwidthEdit = new LineEdit(mPostDistGroup);
    mBandwidthEdit->setFont(font);
    QLocale locale;
    mBandwidthEdit->setText(locale.toString(1.06));
    mBandwidthEdit->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");
    
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
    
    connect(this, &ResultsView::controlsUpdated, this, &ResultsView::updateLayout);
    connect(this, &ResultsView::scalesUpdated, this, &ResultsView::updateControls);
    
    // -------------------------
    
    connect(mTabs, &Tabs::tabClicked, this, &ResultsView::graphTypeChange);
    
    connect(mByPhasesBut, &Button::clicked, this, &ResultsView::changeScrollArea);

    connect(mByEventsBut, &Button::clicked, this, &ResultsView::changeScrollArea);
    

    connect(mPreviousSheetBut, &Button::pressed, this, &ResultsView::previousSheet);
    connect(mNextSheetBut, &Button::pressed, this, &ResultsView::nextSheet);
    
    // -------------------------
    
    connect(mFFTLenCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ResultsView::setFFTLength);
    connect(mBandwidthEdit, &LineEdit::editingFinished, this, &ResultsView::setBandwidth);
    
    connect(mHPDEdit, &LineEdit::editingFinished, this, &ResultsView::setThreshold);

    connect(mDataThetaRadio, &RadioButton::clicked, this, &ResultsView::changeScrollArea);
    connect(mDataSigmaRadio, &RadioButton::clicked, this, &ResultsView::changeScrollArea);
    
    connect(mDataCalibCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mWiggleCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mAllChainsCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mCredibilityCheck, &CheckBox::clicked, this, &ResultsView::generateCurvesRequested);
    
    // -------------------------
    connect(mUnfoldBut, &Button::toggled, this, &ResultsView::unfoldToggle);
    connect(mShowDataUnderPhasesCheck, &CheckBox::toggled, this, &ResultsView::changeScrollArea);
    
    //connect(this, &ResultsView::curvesGenerated, this, &ResultsView::updateControls);
    connect(this, &ResultsView::curvesGenerated, this, &ResultsView::updateCurvesToShow);
    connect(this, &ResultsView::updateScrollAreaRequested, this, &ResultsView::changeScrollArea);
    connect(this, &ResultsView::generateCurvesRequested, this, &ResultsView::updateCurves);
    // -------------------------


    connect(mXSlider, &QSlider::sliderMoved, this, &ResultsView::updateZoomX);
    connect(mXSlider, &QSlider::sliderPressed, this, &ResultsView::updateZoomX);
    connect(mCurrentXMinEdit, &LineEdit::editingFinished, this, &ResultsView::editCurrentMinX);
    connect(mCurrentXMaxEdit, &LineEdit::editingFinished, this, &ResultsView::editCurrentMaxX);
    connect(mRuler, &Ruler::positionChanged, this, &ResultsView::updateScroll);
    
    connect(mYSlider, &QSlider::valueChanged, this, &ResultsView::updateScaleY);
    
    // -------------------------
    
    connect(mRenderCombo,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ResultsView::updateRendering);
    connect(mStatsBut, &Button::toggled, this, &ResultsView::showInfos);
    connect(mExportImgBut, &Button::clicked, this, &ResultsView::exportFullImage);
    connect(mExportResults, &Button::clicked, this, &ResultsView::exportResults);

    mMarker->raise();
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



#pragma mark Options & Layout (Chained Functions)

/**
 * @brief ResultsView::updateControls set controls according to the differents tabs positions.  emit controlsUpdated()
*/
void ResultsView::updateControls()
{
    qDebug() << "ResultsView::updateControls";
    
    bool byEvents = mByEventsBut->isChecked();
    
    /* -------------------------------------------------------
     *  Activate specific controls for post. distrib. (first tab)
     * -------------------------------------------------------*/
    mAllChainsCheck    -> setVisible(mCurrentTypeGraph == GraphViewResults::ePostDistrib);
    mDataCalibCheck    -> setVisible(mCurrentTypeGraph == GraphViewResults::ePostDistrib);
    mWiggleCheck       -> setVisible(mCurrentTypeGraph == GraphViewResults::ePostDistrib);
    mPostDistOptsTitle -> setVisible(mCurrentTypeGraph == GraphViewResults::ePostDistrib);
    mPostDistGroup     -> setVisible(mCurrentTypeGraph == GraphViewResults::ePostDistrib);
    
    /* -------------------------------------------------------
     *  Switch between checkboxes or Radio-buttons for chains
     * -------------------------------------------------------*/
    if (mCurrentTypeGraph == GraphViewResults::ePostDistrib) {
        for (int i=0; i<mCheckChainChecks.size(); ++i)
            mCheckChainChecks.at(i)->setVisible(true);
        
        for (int i=0; i<mChainRadios.size(); ++i)
            mChainRadios.at(i)->setVisible(false);

    } else {
        for (int i=0; i<mCheckChainChecks.size(); ++i)
            mCheckChainChecks.at(i)->setVisible(false);
        
        for (int i=0; i<mChainRadios.size(); ++i)
            mChainRadios.at(i)->setVisible(true);
    }
    
    /* -------------------------------------------------------
     *  Display by phases or by events
     * -------------------------------------------------------*/
    mStack->setCurrentWidget(byEvents ? mEventsScrollArea : mPhasesScrollArea);
    mShowDataUnderPhasesCheck->setVisible(!byEvents);

    /* -------------------------------------------------------
     *  Enable or disable previous and next sheet
     * -------------------------------------------------------*/
    int currentIndex = 0;
    int graphCount = 0;
    if (mStack->currentWidget() == mEventsScrollArea) {
        graphCount = mModel->mNumberOfEvents;
        if (mUnfoldBut->isChecked())
            graphCount += mModel->mNumberOfDates;
        currentIndex = mTabEventsIndex;
    }

    else if (mStack->currentWidget() == mPhasesScrollArea) {
        graphCount = mModel->mNumberOfPhases;
        if (mUnfoldBut->isChecked()) {
            graphCount += mModel->mNumberOfEventsInAllPhases;
            if (mShowDataUnderPhasesCheck->isVisible() && mShowDataUnderPhasesCheck->isChecked())
                graphCount += mModel->mNumberOfDatesInAllPhases;
        }
        currentIndex = mTabPhasesIndex;
    }

    if (currentIndex == 0)
        mPreviousSheetBut->setEnabled(false);
    else
        mPreviousSheetBut->setEnabled(true);

    if ( ((currentIndex+1)*mNumberOfGraph) < graphCount)
        mNextSheetBut->setEnabled(true);
    else
        mNextSheetBut->setEnabled(false);


    qDebug() << "ResultsView::updateControls -> emit controlsUpdated()";
    emit controlsUpdated();
}

void ResultsView::changeScrollArea()
{
    if (mByEventsBut->isChecked())
            createEventsScrollArea(mTabEventsIndex);

    else if (mByPhasesBut->isChecked())
            createPhasesScrollArea(mTabPhasesIndex);
}

void ResultsView::updateLayout()
{
    if (!mModel)
        return;

    qDebug() << "ResultsView::updateLayout()";
    
    const int sbe = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    int graphYAxis = 50;
    const int m = mMargin;
    int dx = mLineH + mMargin;
    
    const int tabIdx = mTabs->currentIndex();
    
    mCurrentXMinEdit->setText( stringWithAppSettings(mResultCurrentMinX) );
    mCurrentXMaxEdit->setText( stringWithAppSettings(mResultCurrentMaxX) );
    /* ----------------------------------------------------------
     *  Main layout
     * ----------------------------------------------------------*/
    mByPhasesBut->setGeometry(0, 0, int(mGraphLeft/2), mTabsH);
    mByEventsBut->setGeometry(int(mGraphLeft/2), 0, int(mGraphLeft/2), mTabsH);
    mUnfoldBut->setGeometry(0, mTabsH, mGraphLeft, int(mRulerH/2));
    mPreviousSheetBut->setGeometry(0, mTabsH + int(mRulerH/2), int(mGraphLeft/2), int(mRulerH/2) );
    mNextSheetBut->setGeometry(int(mGraphLeft/2),  mTabsH + int(mRulerH/2), int(mGraphLeft/2), int(mRulerH/2));
    
    mTabs->setGeometry(mGraphLeft + graphYAxis, 0, width() - mGraphLeft - mOptionsW - sbe - graphYAxis, mTabsH);
    mRuler->setGeometry(mGraphLeft, mTabsH, width() - mGraphLeft - mOptionsW - sbe, mRulerH);

    mStack->setGeometry(0, mTabsH + mRulerH, width() - mOptionsW, height() - mRulerH - mTabsH);
    mMarker->setGeometry(mMarker->pos().x(), mTabsH + sbe, mMarker->thickness(), height() - sbe - mTabsH);
    
    mOptionsWidget->setGeometry(this->width() - mOptionsW, 0, mOptionsW, this->height());
    
    /* ----------------------------------------------------------
     *  Display Options options layout
     * ----------------------------------------------------------*/
    mDisplayGroup->setGeometry(0, mDisplayTitle->y()+ mDisplayTitle->height(), mOptionsW, mDisplayGroup->height());
    
    
    /* ----------------------------------------------------------
     *  MCMC Chains options layout
     * ----------------------------------------------------------*/

    int numChains = mCheckChainChecks.size();
    
    // posterior distribution : chains are selectable with checkboxes
    if (tabIdx == 0) {
        mChainsGroup->setFixedHeight(m + (numChains+1) * (mLineH + m));
        mAllChainsCheck->setGeometry(m, m, int(mChainsGroup->width()-2*m), mLineH);

        for (int i=0; i<numChains; ++i) {
            QRect geometry(m, m + (i+1) * (mLineH + m), int(mChainsGroup->width()-2*m), mLineH);
            mCheckChainChecks.at(i)->setGeometry(geometry);
            mChainRadios.at(i)->setGeometry(geometry);
        }

    // trace, accept or correl : chains are selectable with radio-buttons
    } else {
        mChainsGroup->setFixedHeight(m + numChains * (mLineH + m));
        for (int i=0; i<numChains; ++i) {
            QRect geometry(m, int(m + i * (mLineH + m)), int(mChainsGroup->width()-2*m), mLineH);
            mCheckChainChecks.at(i) -> setGeometry(geometry);
            mChainRadios.at(i)     -> setGeometry(geometry);
        }
    }
    
    /* ----------------------------------------------------------
     *  Results options layout
     * ----------------------------------------------------------*/
    int y = m;
    mDataThetaRadio->setGeometry(m, y, int(mResultsGroup->width() - 2*m), mLineH);
    
    // posterior distribution
    if (tabIdx == 0) {
        mDataCalibCheck -> setGeometry(m + dx, y += (m + mLineH), int(mResultsGroup->width() - 2*m - dx), mLineH);
        mWiggleCheck    -> setGeometry(m + dx, y += (m + mLineH), int( mResultsGroup->width() - 2*m - dx), mLineH);
    }

    if (mByPhasesBut->isChecked())
        mShowDataUnderPhasesCheck->setGeometry(m + dx, y += (m + mLineH), int(mResultsGroup->width() - 2*m - dx), mLineH);

    mDataSigmaRadio -> setGeometry(m, y += (m + mLineH), mResultsGroup->width()-2*m, mLineH);
    mResultsGroup   -> setFixedHeight(y += (m + mLineH));
    
    /* ----------------------------------------------------------
     *  Post. Distrib. options layout
     * ----------------------------------------------------------*/
    mPostDistGroup->setFixedWidth(mOptionsW);
    
    y = m;
    int sw = int((mPostDistGroup->width() - 3*m) * 0.5);
    int w1 = int((mPostDistGroup->width() - 3*m) * 0.7);
    int w2 = int((mPostDistGroup->width() - 3*m) * 0.3);
    
    mCredibilityCheck->setGeometry(m, y, mPostDistGroup->width() - 2*m, mLineH);
    mThreshLab->setGeometry(m, y += (m + mLineH), w1, mLineH);
    mHPDEdit->setGeometry(2*m + w1, y, w2, mLineH);
    mFFTLenLab->setGeometry(m, y += (m + mLineH), sw, mComboH);
    mFFTLenCombo->setGeometry(2*m + sw, y, sw, mComboH);
    mBandwidthLab->setGeometry(m, y += (m + mComboH), w1, mLineH);
    mBandwidthEdit->setGeometry(2*m + w1, y, w2, mLineH);
    mPostDistGroup->setFixedHeight(y += (m + mLineH));
    
    updateGraphsLayout();
}

void ResultsView::updateGraphsLayout()
{
    qDebug() << "ResultsView::updateGraphsLayout()";
    const int sbe = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    
    /* ----------------------------------------------------------
     *  Graphs by phases layout
     *    mRuler  -> setGeometry(mGraphLeft, mTabsH, width() - mGraphLeft - mOptionsW - sbe, mRulerH);
     * ----------------------------------------------------------*/
    if (mByPhasesBut->isChecked()) {
        int y = 0;
        if (mPhasesScrollArea) {
            QWidget* wid = mPhasesScrollArea->widget();
            for (int i=0; i<mByPhasesGraphs.size(); ++i) {
                mByPhasesGraphs.at(i)->setGeometry(0, y, width() - mOptionsW - sbe ,mGraphsH);
                y += mByPhasesGraphs.at(i)->height();
            }
            if (y>0)
                wid->setFixedSize(width() - sbe - mOptionsW, y);

       }
        
    }
    /* ----------------------------------------------------------
     *  Graphs by events layout
     * ----------------------------------------------------------*/
   
    else {
         int y = 0;
         if (mEventsScrollArea) {
            QWidget* wid = mEventsScrollArea->widget();

            for (int i=0; i<mByEventsGraphs.size(); ++i) {
                mByEventsGraphs.at(i)->setGeometry(0, y, width() - mOptionsW - sbe ,mGraphsH);
                y += mByEventsGraphs.at(i)->height();
            }
            if (y>0)
                wid->setFixedSize(width() - sbe - mOptionsW, y);
         }
        
    }
    update();
    
}

//pragma mark Update (Chained Functions)
void ResultsView::clearResults()
{
    mByEventsBut->setEnabled(false);
    mByPhasesBut->setEnabled(false);

    for (auto&& check : mCheckChainChecks ) {
        delete check;
        check = nullptr;
    }
    mCheckChainChecks.clear();
    
    for (auto&& chain : mChainRadios) {
        delete chain;
        chain = nullptr;
    }
    mChainRadios.clear();

    for (auto&& graph : mByEventsGraphs) {
        delete graph;
        graph = nullptr;
    }
    mByEventsGraphs.clear();
    
    for (auto&& graph : mByPhasesGraphs) {
        delete graph;
        graph = nullptr;
    }
    mByPhasesGraphs.clear();

    if (mEventsScrollArea) {
        mStack->removeWidget(mEventsScrollArea);
        QWidget* wid = mEventsScrollArea->widget();
        delete wid;
        delete mEventsScrollArea;
    }
    if (mPhasesScrollArea){
        mStack->removeWidget(mPhasesScrollArea);
        delete mPhasesScrollArea;
    }
    mEventsScrollArea = nullptr;
    mPhasesScrollArea = nullptr;

    mResultMinX = mSettings.getTminFormated();
    mResultMaxX = mSettings.getTmaxFormated();

    mResultCurrentMinX = mResultMinX ;
    mResultCurrentMaxX = mResultMaxX ;
    mResultZoomX = 100;
    mRuler->setCurrent(mResultCurrentMinX, mResultCurrentMaxX);
    updateZoomEdit();
}


void ResultsView::updateFormatSetting(Model* model, const AppSettings* appSet)
{
    if (!mModel && !model)
        return;
    if (model)
        mModel = model;
    mModel->updateFormatSettings(appSet);
    mNumberOfGraph = appSet->mNbSheet;
}

/**
 * @brief : This function is call after "Run"
 *
 */
void ResultsView::initResults(Model* model)
{
    clearResults();

    qDebug() << "ResultsView::initResults";

    if (!mModel && !model)
        return;

    if (model)
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
    if (mCheckChainChecks.isEmpty()) {
        for (int i=0; i<mChains.size(); ++i) {
            CheckBox* check = new CheckBox(tr("Chain") + " " + QString::number(i+1), mChainsGroup);
            connect(check, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
            check->setVisible(true);
            mCheckChainChecks.append(check);

            RadioButton* radio = new RadioButton(tr("Chain") + " " + QString::number(i+1), mChainsGroup);
            connect(radio, &RadioButton::clicked, this, &ResultsView::updateCurvesToShow);
            radio->setVisible(true);
            if (i == 0)
                radio->setChecked(true);
            mChainRadios.append(radio);
        }
    }

    // ------------------------------------------------------------
    //  This generates post. densities, HPD and credibilities !
    //  It will then call in chain :
    //  - generateCredibilityAndHPD
    //  - generateCurves
    //  - updateCurvesToShow
    // ------------------------------------------------------------
    mModel->initDensities(getFFTLength(), getBandwidth(), getThreshold());

    if (mHasPhases)
        createPhasesScrollArea(0);
    else
        createEventsScrollArea(0);

    // ------------------------------------------------------------
   // updateGraphsZoomX(); // to set the CurrentMinX value inside the graphView
    showInfos(mStatsBut->isChecked());


}
/**
 * @brief This function is call after click on "Results", when switching from Model panel to Result panel
 * @brief Currently this function is identical to initResults()
 */
void ResultsView::updateResults(Model* model)
{
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
        //  Update Chains option controls (radio and checkboxes under "MCMC Chains")
        // ----------------------------------------------------
        if (mCheckChainChecks.isEmpty()) {
            for (int i = 0; i<mChains.size(); ++i) {
                CheckBox* check = new CheckBox(tr("Chain") + " " + QString::number(i+1), mChainsGroup);
                connect(check, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
                check->setVisible(true);
                mCheckChainChecks.append(check);

                RadioButton* radio = new RadioButton(tr("Chain") + " " + QString::number(i+1), mChainsGroup);
                connect(radio, &RadioButton::clicked, this, &ResultsView::updateCurvesToShow);
                radio->setVisible(true);
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

        // ------------------------------------------------------------
        //  This generates post. densities, HPD and credibilities !
        //  It will then call in chain :
        //  - generateCredibilityAndHPD
        //  - generateCurves
        //  - updateCurvesToShow
        // ------------------------------------------------------------

        //mModel->initDensities(getFFTLength(), getBandwidth(), getThreshold());
        mModel->updateDensities(getFFTLength(), getBandwidth(), getThreshold());
        // ----------------------------------------------------
        //  Events Views : generate all phases graph
        //  No posterior density has been computed yet!
        //  Graphs are empty at the moment
        // ----------------------------------------------------
        if (mHasPhases) createPhasesScrollArea(mTabPhasesIndex);
        else createEventsScrollArea(mTabEventsIndex);

        // ------------------------------------------------------------

        showInfos(mStatsBut->isChecked());

}


void ResultsView::createEventsScrollArea(const int idx)
{
   /* GraphViewResults::Variable variable = GraphViewResults::eTheta;;
    if(mDataThetaRadio->isChecked())
        variable = GraphViewResults::eTheta;
    else if(mDataSigmaRadio->isChecked())
        variable = GraphViewResults::eSigma;
*/

    qDebug()<<"ResultsView::createEventsScrollArea()";
    if (!mEventsScrollArea) {
        mEventsScrollArea = new QScrollArea();
        mEventsScrollArea->setMouseTracking(true);
        mStack->addWidget(mEventsScrollArea);
    }

    for (auto g : mByEventsGraphs)
        delete g;

    mByEventsGraphs.clear();


    // eventsWidget Creation in the idx limit
    QWidget* eventsWidget = new QWidget(mEventsScrollArea);
    eventsWidget->setMouseTracking(true);

    QList<Event*>::const_iterator iterEvent = mModel->mEvents.cbegin();
    int counter = 1;

    while (iterEvent!=mModel->mEvents.cend()) {
        if ( (idx*mNumberOfGraph)<counter && counter <= ((idx+1)*mNumberOfGraph) ) {
            GraphViewEvent* graphEvent = new GraphViewEvent(eventsWidget);

            graphEvent->setSettings(mModel->mSettings);
            graphEvent->setMCMCSettings(mModel->mMCMCSettings, mChains);
            graphEvent->setEvent((*iterEvent));
            graphEvent->setGraphFont(mFont);
            graphEvent->setGraphsThickness(mThicknessSpin->value());
            mByEventsGraphs.append(graphEvent);
          }
        ++counter; //count one event graph
        if (mUnfoldBut->isChecked()) {
                if ((*iterEvent)->mType != Event::eKnown) {
                    for (int j=0; j<(*iterEvent)->mDates.size(); ++j) {
                        if ( (idx*mNumberOfGraph)<counter && counter <= (idx+1)*mNumberOfGraph) {
                            Date& date = (*iterEvent)->mDates[j];
                            // ----------------------------------------------------
                            //  This just creates the view for the date.
                            //  It sets the Date which triggers an update() to repaint the view.
                            //  The refresh() function which actually creates the graph curves will be called later.
                            // ----------------------------------------------------

                            GraphViewDate* graphDate = new GraphViewDate(eventsWidget);
                            graphDate->setSettings(mModel->mSettings);
                            graphDate->setMCMCSettings(mModel->mMCMCSettings, mChains);
                            graphDate->setDate(&date);
                            graphDate->setColor((*iterEvent)->mColor);

                            graphDate->setGraphFont(mFont);
                            graphDate->setGraphsThickness(mThicknessSpin->value());
                            graphDate->setGraphsOpacity(mOpacitySpin->value());
                            mByEventsGraphs.append(graphDate);

                        }
                        ++counter;
                    }
                }

        }

        ++iterEvent;
    }

    
    mEventsScrollArea->setWidget(eventsWidget);

    mEventsScrollArea->update();
    qDebug()<<"ResultsView::createEventsScrollArea()"<<counter<<" items";
    
    emit generateCurvesRequested();

}

void ResultsView::createPhasesScrollArea(const int idx)
{
    qDebug()<<"ResultsView::createPhasesScrollArea()";
    
  /*  GraphViewResults::Variable variable = GraphViewResults::eTheta;;
    if(mDataThetaRadio->isChecked())
        variable = GraphViewResults::eTheta;
    else if(mDataSigmaRadio->isChecked())
        variable = GraphViewResults::eSigma;
*/
    
    if (!mPhasesScrollArea) {
        mPhasesScrollArea = new QScrollArea(this);
        mPhasesScrollArea->setMouseTracking(true);
        mStack->addWidget(mPhasesScrollArea);
    }
    for (auto g : mByPhasesGraphs)
        delete g;
    mByPhasesGraphs.clear();


    QWidget* phasesWidget = new QWidget(this);
    phasesWidget->setMouseTracking(true);
    
    // In a Phases at least, we have one Event with one Date
    //mByPhasesGraphs.reserve( (int)(3*mModel->mPhases.size()) );

    QList<Phase*>::const_iterator iterPhase = mModel->mPhases.cbegin();
    int counter = 1;
    while (iterPhase!=mModel->mPhases.cend()) {
        if ( (idx*mNumberOfGraph)<counter && counter <= ((idx+1)*mNumberOfGraph) ) {
            GraphViewPhase* graphPhase = new GraphViewPhase(phasesWidget);
            //graphPhase->setUpdatesEnabled(false);
            graphPhase->setSettings(mModel->mSettings);
            graphPhase->setMCMCSettings(mModel->mMCMCSettings, mChains);
            graphPhase->setPhase((*iterPhase));
            graphPhase->setGraphFont(mFont);
            graphPhase->setGraphsThickness(mThicknessSpin->value());
            mByPhasesGraphs.append(graphPhase);
         }
        ++ counter;//count one phase graph
        if (mUnfoldBut->isChecked()) {

            QList<Event*>::const_iterator iterEvent = (*iterPhase)->mEvents.cbegin();
            while (iterEvent!=(*iterPhase)->mEvents.cend()) {
                if ( (idx*mNumberOfGraph)<counter && counter <= (idx+1)*mNumberOfGraph) {
                    GraphViewEvent* graphEvent = new GraphViewEvent(phasesWidget);
                    graphEvent->setSettings(mModel->mSettings);
                    graphEvent->setMCMCSettings(mModel->mMCMCSettings, mChains);
                    graphEvent->setEvent((*iterEvent));
                    graphEvent->setGraphFont(mFont);
                    graphEvent->setGraphsThickness(mThicknessSpin->value());
                    mByPhasesGraphs.append(graphEvent);
                }
                ++ counter; // count one Event
                // --------------------------------------------------
                //  This just creates the GraphView for the date (no curve yet)
                // --------------------------------------------------
                if (mShowDataUnderPhasesCheck->isChecked()) {
                    for (int j=0; j<(*iterEvent)->mDates.size(); ++j) {
                        if ( (idx*mNumberOfGraph)<counter && counter <= (idx+1)*mNumberOfGraph) {
                            Date& date = (*iterEvent)->mDates[j];
                            GraphViewDate* graphDate = new GraphViewDate(phasesWidget);
                            //graphDate->setUpdatesEnabled(false);
                            graphDate->setSettings(mModel->mSettings);
                            graphDate->setMCMCSettings(mModel->mMCMCSettings, mChains);
                            graphDate->setDate(&date);
                            graphDate->setColor((*iterEvent)->mColor);
                            graphDate->setGraphFont(mFont);
                            graphDate->setGraphsThickness(mThicknessSpin->value());

                            mByPhasesGraphs.append(graphDate);
                        }
                        ++ counter; //count one Date
                    }
                }
                ++iterEvent;
            }
        }
        ++iterPhase;
    }

       //phasesWidget->setUpdatesEnabled(true);
    mPhasesScrollArea->setWidget(phasesWidget);
    mPhasesScrollArea->update();
    
    qDebug()<<"ResultsView::createPhasesScrollArea()"<<counter<<" items";
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
    const QLocale locale;
    bool ok;
    const double bandwidth = locale.toDouble(mBandwidthEdit->text(), &ok);
    if (!(bandwidth > 0 && bandwidth <= 100) || !ok)
        mBandwidthEdit->setText(locale.toString(bandwidth));
    
    if (bandwidth != getBandwidth()) {
        mBandwidthUsed = bandwidth;
        mModel->setBandwidth(bandwidth);
    }
    
    //emit updateScrollAreaRequested();
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
    //emit updateScrollAreaRequested();
}

void ResultsView::unfoldToggle()
{
    
    qreal graphInit = 0.;
    qreal graphTarget = 0.;
    
    if (mStack->currentWidget() == mEventsScrollArea) {
        
        // number of Items increase
        if (mUnfoldBut->isChecked()) {
            graphInit = (qreal) mModel->mNumberOfEvents;
            graphTarget = (qreal) mModel->mNumberOfEvents + mModel->mNumberOfDates;
            
        } else {
            graphInit = (qreal) mModel->mNumberOfEvents + mModel->mNumberOfDates;
            graphTarget = (qreal) mModel->mNumberOfEvents;
        }
            
        mTabEventsIndex = (int)((mTabEventsIndex*mNumberOfGraph / graphInit) * graphTarget/mNumberOfGraph);
        
    } else if (mStack->currentWidget() == mPhasesScrollArea) {
        
            // number of Items increase
            if (mUnfoldBut->isChecked()) {
                graphInit = (qreal) mModel->mNumberOfPhases;
                // number of Items increase
                if (mShowDataUnderPhasesCheck->isVisible() && mShowDataUnderPhasesCheck->isChecked())
                    graphTarget = (qreal) (mModel->mNumberOfPhases + mModel->mNumberOfEvents + mModel->mNumberOfDates) ;
                else
                    graphTarget = (qreal) (mModel->mNumberOfPhases + mModel->mNumberOfEvents) ;
                
            
            } else {
                if (mShowDataUnderPhasesCheck->isVisible() && mShowDataUnderPhasesCheck->isChecked())
                    graphInit = (qreal) (mModel->mNumberOfPhases + mModel->mNumberOfEvents + mModel->mNumberOfDates) ;
                else
                    graphInit = (qreal) (mModel->mNumberOfPhases + mModel->mNumberOfEvents) ;
                
                
                graphTarget =  (qreal) (mModel->mNumberOfPhases);
            }
        
       mTabPhasesIndex = (int)((mTabPhasesIndex*mNumberOfGraph / graphInit) * graphTarget/mNumberOfGraph);
        
    }
    
    emit updateScrollAreaRequested();
}

void ResultsView::nextSheet()
{
    
    int* currentIndex = nullptr;
    int graphCount = 0;
    
    if (mStack->currentWidget() == mEventsScrollArea) {
        graphCount = mModel->mNumberOfEvents;
        if (mUnfoldBut->isChecked())
            graphCount += mModel->mNumberOfDates;
        currentIndex = &mTabEventsIndex;
    }
    
    else if (mStack->currentWidget() == mPhasesScrollArea) {
        graphCount = mModel->mNumberOfPhases;
        if (mUnfoldBut->isChecked()) {
            graphCount += mModel->mNumberOfEventsInAllPhases;
            if (mShowDataUnderPhasesCheck->isVisible() && mShowDataUnderPhasesCheck->isChecked())
                graphCount += mModel->mNumberOfDatesInAllPhases;
        }
        currentIndex = &mTabPhasesIndex;
    }
  
    if ( ((*currentIndex)*mNumberOfGraph) < graphCount)
        (*currentIndex)++;
    
    
    emit updateScrollAreaRequested();
    
}

void ResultsView::previousSheet()
{
    if (mByEventsBut->isChecked() && (mTabEventsIndex>0))
        --mTabEventsIndex;

    else if (mByPhasesBut->isChecked() && (mTabPhasesIndex>0))
        --mTabPhasesIndex;
    
    emit updateScrollAreaRequested();
}

/**
 *  @brief Decide which curve graphs must show, based on currently selected options.
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
    const bool showStat = mStatsBut->isChecked();

    if (mByPhasesBut->isChecked() )
        for (GraphViewResults* phaseGraph : mByPhasesGraphs) {
                phaseGraph->updateCurvesToShow(showAllChains, showChainList, showCredibility, showCalib, showWiggle);
                phaseGraph->setShowNumericalResults(showStat);
        }
    else
        for (GraphViewResults* eventGraph : mByEventsGraphs) {
            eventGraph->updateCurvesToShow(showAllChains, showChainList, showCredibility, showCalib, showWiggle);
            eventGraph->setShowNumericalResults(showStat);
        }

    updateScales();
}



/**
 *  @brief re-generate all curves in graph views form model data.
 *  @brief Each curve is given a name. This name will be used by updateCurvesToShow() to decide whether the curve is visible or not.
 */
void ResultsView::generateCurves(const QList<GraphViewResults*>& listGraphs)
{
    qDebug() << "ResultsView::generateCurves()";

    GraphViewResults::Variable variable = GraphViewResults::eTheta;;
    if (mDataThetaRadio->isChecked())
        variable = GraphViewResults::eTheta;

    else if (mDataSigmaRadio->isChecked())
        variable = GraphViewResults::eSigma;

    QList<GraphViewResults*>::const_iterator constIter = listGraphs.cbegin();
    QList<GraphViewResults*>::const_iterator iterEnd = listGraphs.cend();

    while(constIter != iterEnd) {
        (*constIter)->generateCurves(GraphViewResults::TypeGraph(mCurrentTypeGraph), variable);
        ++constIter;
    }


    // With variable eSigma, we look for mResultMaxVariance in the curve named "Post Distrib All Chains"
    if (variable == GraphViewResults::eSigma) {
        mResultMaxVariance = 0.;

        QList<GraphViewResults*>::const_iterator constIter;
        constIter = listGraphs.cbegin();
        while (constIter != iterEnd) {
            const GraphViewEvent* graphEvent = dynamic_cast<const GraphViewEvent*>(*constIter);
            
            if (graphEvent) {
                const int nbCurves = graphEvent->mGraph->numCurves();
                // if we have Curves there are only Variance curves
                for (int i=0; i <nbCurves; ++i) {
                    const GraphCurve* graphVariance = graphEvent->mGraph->getCurve("Sigma Date " + QString::number(i) + " All Chains");
                    if (graphVariance)
                        mResultMaxVariance = ceil(qMax(mResultMaxVariance, graphVariance->mData.lastKey()));
                }
            } else {            
                const GraphViewDate* graphDate = dynamic_cast<const GraphViewDate*>(*constIter);
                if (graphDate) {
                    const GraphCurve* graphVariance = graphDate->mGraph->getCurve("Sigma All Chains");
                    if (graphVariance)
                        mResultMaxVariance = ceil(qMax(mResultMaxVariance, graphVariance->mData.lastKey()));

                }
            }
            ++constIter;
        }

    }
     qDebug() << "ResultsView::generateCurves()-> emit curvesGenerated()";
    emit curvesGenerated();
}

void ResultsView::updateCurves()
{
     qDebug() << "ResultsView::updateCurves()";
    if (mByPhasesBut->isChecked())
        generateCurves(mByPhasesGraphs);
    else
        generateCurves(mByEventsGraphs);

    qDebug() << "ResultsView::updateCurves()-> emit curvesGenerated()";
   emit curvesGenerated();
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
    
    /* ------------------------------------------
     *  Get X Range based on current options
     * ------------------------------------------*/
    if (tabIdx == 0) {
        if (mDataThetaRadio->isChecked()) {
            mResultMinX = s.getTminFormated();
            mResultMaxX = s.getTmaxFormated();
        } else if (mDataSigmaRadio->isChecked())  {
            mResultMinX = 0.;
            mResultMaxX = mResultMaxVariance;

        }
    } else if (tabIdx == 1 || tabIdx == 2) {
        mResultMinX = 0.;
        for (int i = 0; i < mChainRadios.size(); ++i) {
            if (mChainRadios.at(i)->isChecked()) {
                const ChainSpecs& chain = mChains.at(i);
                mResultMaxX = 1+ chain.mNumBurnIter + (chain.mBatchIndex * chain.mNumBatchIter) + chain.mNumRunIter / chain.mThinningInterval;
                break;
            }
        }
    } else if (tabIdx == 3) {
        mResultMinX = 0.;
        mResultMaxX = 39.;
    }
   
    /* ------------------------------------------
     *  Restore last zoom values; must be stored in unformated value
     * ------------------------------------------*/
    if (mZooms.find(tabIdx) != mZooms.end()) {
        if (tabIdx == 1) {
            auto currentMinMax = std::minmax(DateUtils::convertToAppSettingsFormat(mZooms.value(tabIdx).first),
                                             DateUtils::convertToAppSettingsFormat(mZooms.value(tabIdx).second));
            mResultCurrentMinX = currentMinMax.first;
            mResultCurrentMaxX = currentMinMax.second;
        } else {
            mResultCurrentMinX = mZooms.value(tabIdx).first;
            mResultCurrentMaxX = mZooms.value(tabIdx).second;

        }
        // controle if the current value is in rigth range depending to mDataThetaRadio and mDataSigmaRadio
        mResultCurrentMinX = qBound(mResultMinX, mResultCurrentMinX, mResultMaxX);
        mResultCurrentMaxX = qBound(mResultCurrentMinX, mResultCurrentMaxX, mResultMaxX);

    } else {
        mResultCurrentMinX = mResultMinX;
        mResultCurrentMaxX = mResultMaxX;
    }

    mResultZoomX = (mResultCurrentMaxX - mResultCurrentMinX) / (mResultMaxX - mResultMinX) * 100.;
    
    
    
    /* -----------------------------------------------
     *  Set All Graphs Ranges (This is not done by generateCurves !)
     * -----------------------------------------------*/
    if (mByPhasesBut->isChecked()) {
        for (GraphViewResults* phaseGraph : mByPhasesGraphs) {
            phaseGraph->setRange(mResultMinX, mResultMaxX);
            phaseGraph->setCurrentX(mResultCurrentMinX, mResultCurrentMaxX);
            phaseGraph->zoom(mResultCurrentMinX, mResultCurrentMaxX);
        }
        //adjustDuration(true);
    } else
        for (GraphViewResults* eventGraph : mByEventsGraphs) {
            eventGraph->setRange(mResultMinX, mResultMaxX);
            eventGraph->setCurrentX(mResultCurrentMinX, mResultCurrentMaxX);
            eventGraph->zoom(mResultCurrentMinX, mResultCurrentMaxX);
        }
    /* ------------------------------------------
     *  Set Zoom Slider & Zoom Edit
     * ------------------------------------------*/
    int zoom = 100 - (int)mResultZoomX;
    mXSlider->setValue(zoom);
    updateZoomEdit();
    
    // Already done when setting graphs new range (above)
    //updateGraphsZoomX();
    
    /* ------------------------------------------
     *  Set Ruler Range
     * ------------------------------------------*/
    mRuler->setFormatFunctX(stringWithAppSettings);

    mRuler->setRange(mResultMinX, mResultMaxX);
    mRuler->setCurrent(mResultCurrentMinX, mResultCurrentMaxX);
    
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
    emit scalesUpdated();
}

//#pragma mark Log results
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
        for (auto&& event : mModel->mEvents)
            log += ModelUtilities::eventResultsHTML(event, true, mModel);

        for (auto&& phase : mModel->mPhases)
            log += ModelUtilities::phaseResultsHTML(phase);

        for (auto&& phaseConstraint : mModel->mPhaseConstraints) {
            log += ModelUtilities::constraintResultsHTML(phaseConstraint);
            log += "<hr>";
        }
    } catch (std::exception const & e) {
        qDebug()<< "in ResultsView::updateResultsLog() Error"<<e.what();
        log = tr("impossible to compute");
    }
qDebug()<< "ResultsView::updateResultsLog()-> emit resultsLogUpdated(log)";
    emit resultsLogUpdated(log);
}

//#pragma mark Mouse & Marker
void ResultsView::mouseMoveEvent(QMouseEvent* e)
{
    int shiftX (0);
    
    int x = e->pos().x() - shiftX;
    x = (x >= mGraphLeft) ? x : mGraphLeft;
    x = (x <= width() - mOptionsW) ? x : width() - mOptionsW;
    mMarker->setGeometry(x, mMarker->pos().y(), mMarker->width(), mMarker->height());
}

#pragma mark Zoom X
void ResultsView::updateZoomX()
{
    /* --------------------------------------------------
     *  Find new current min & max, update mResultZoomX
     * --------------------------------------------------*/
    int zoom = mXSlider->value();
    
    // Ici, 10 correspond à la différence minimale de valeur (quand le zoom est le plus fort)
    double minProp = 10. / (mResultMaxX - mResultMinX);
    double zoomProp = (100. - zoom) / 100.;
    if (zoomProp < minProp)
        zoomProp = minProp;
    zoom = 100 * (1 - zoomProp);
    
    mResultZoomX = (double)zoom;
    double span = (mResultMaxX - mResultMinX)* (100.-mResultZoomX)/100.;
    double mid = (mResultCurrentMaxX + mResultCurrentMinX)/2.;
    double curMin = mid - span/2.;
    double curMax = mid + span/2.;
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
    
    if (zoom == 0) {
        mResultCurrentMinX = mResultMinX;
        mResultCurrentMaxX = mResultMaxX;
    }
    
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
    QString str = mCurrentXMinEdit->text();
    bool isNumber;
    double value = str.toDouble(&isNumber);

    if (isNumber) {
        double current = qBound(mResultMinX, value, mResultCurrentMaxX);
        if (current == mResultCurrentMaxX) {
            current = mResultMinX;
        }
        mResultCurrentMinX = current;
        mResultZoomX = (mResultCurrentMaxX - mResultCurrentMinX)/ (mResultMaxX - mResultMinX) * 100.;
        
        /* --------------------------------------------------
         *  Update other elements
         * --------------------------------------------------*/
        int zoom = int(100-mResultZoomX);
        mXSlider->setValue(zoom);
        
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
    bool isNumber;
    double value = str.toDouble(&isNumber);

    if (isNumber) {

        double current = qBound(mResultCurrentMinX, value, mResultMaxX);
        if (current == mResultCurrentMinX)
            current = mResultMaxX;
        
        mResultCurrentMaxX = current;
        mResultZoomX = (mResultCurrentMaxX - mResultCurrentMinX)/ (mResultMaxX - mResultMinX)* 100.;
        
        /* --------------------------------------------------
         *  Update other elements
         * --------------------------------------------------*/
        int zoom = int(100-mResultZoomX);
        mXSlider->setValue(zoom);
        
        mRuler->setCurrent(mResultCurrentMinX, mResultCurrentMaxX);
        updateZoomEdit();
        updateGraphsZoomX();
    }
}

void ResultsView::updateZoomEdit()
{
    mCurrentXMinEdit->setText(stringWithAppSettings(mResultCurrentMinX));
    mCurrentXMaxEdit->setText(stringWithAppSettings(mResultCurrentMaxX));
}

void ResultsView::updateGraphsZoomX()
{
    if (mByPhasesBut->isChecked())
        for (GraphViewResults* phaseGraph : mByPhasesGraphs)
            phaseGraph->zoom(mResultCurrentMinX, mResultCurrentMaxX);
    
    else
        for (GraphViewResults* eventGraph : mByEventsGraphs)
            eventGraph->zoom(mResultCurrentMinX, mResultCurrentMaxX);
    
    /* --------------------------------------------------
     * Store zoom values in an unformated value AD/BD for Post. distrib tab
     * --------------------------------------------------*/
    int tabIdx = mTabs->currentIndex();
    if (tabIdx == 1) {
        auto resultMinMax = std::minmax( DateUtils::convertFromAppSettingsFormat(mResultCurrentMinX),
                                     DateUtils::convertFromAppSettingsFormat(mResultCurrentMaxX));
        mZooms[tabIdx] = QPair<double, double>(resultMinMax.first, resultMinMax.second);
    } else
        mZooms[tabIdx] = QPair<double, double>(mResultCurrentMinX, mResultCurrentMaxX);

}

//#pragma mark Zoom Y
void ResultsView::updateScaleY(int value)
{
    const double min = 70.;
    const double max = 1070.;
    const double prop = value / 100.;
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
    const QFont font = QFontDialog::getFont(&ok, mFont, this);
    if (ok) {
        mFont = font;
        mFontBut->setText(mFont.family() + ", " + QString::number(mFont.pointSizeF()));
        
        for (GraphViewResults* phaseGraph : mByPhasesGraphs)
            phaseGraph->setGraphFont(mFont);
        
        //mPhasesScrollArea->setFont(mFont); //unnecessary

         for (GraphViewResults* eventGraph : mByEventsGraphs)
            eventGraph->setGraphFont(mFont);

         mRuler->setFont(mFont);

        //mEventsScrollArea->setFont(mFont);//unnecessary

    }
}

void ResultsView::updateThickness(int value)
{
    for (GraphViewResults* allKindGraph : mByPhasesGraphs) {
        allKindGraph->setGraphsThickness(value);
        GraphViewPhase* phaseGraphs = dynamic_cast<GraphViewPhase*>(allKindGraph);
        
        if (phaseGraphs)
            phaseGraphs->mDurationGraph->setGraphsThickness(value);
        
    }

    for (GraphViewResults* allKindGraph : mByEventsGraphs)
        allKindGraph->setGraphsThickness(value);
    
}

void ResultsView::updateOpacity(int value)
{
    for (GraphViewResults* allKindGraph : mByPhasesGraphs) {
       allKindGraph->setGraphsOpacity(value);
       GraphViewPhase* phaseGraphs = dynamic_cast<GraphViewPhase*>(allKindGraph);
        
       if (phaseGraphs)
           phaseGraphs->mDurationGraph->setCurvesOpacity(value);
    }
    
    for (GraphViewResults* allKindGraph : mByEventsGraphs)
        allKindGraph->setGraphsOpacity(value);
    
}

void ResultsView::updateRendering(int index)
{
    for (GraphViewResults* allKindGraph : mByPhasesGraphs) {
        allKindGraph->setRendering((GraphView::Rendering) index);
        GraphViewPhase* phaseGraphs = dynamic_cast<GraphViewPhase*>(allKindGraph);
        
        if(phaseGraphs)
            phaseGraphs->mDurationGraph->setRendering((GraphView::Rendering) index);
    }
    
    for (GraphViewResults* allKindGraph : mByEventsGraphs)
        allKindGraph->setRendering((GraphView::Rendering) index);
    
}

void ResultsView::showInfos(bool show)
{
    for (GraphViewResults* allKindGraph : mByPhasesGraphs)
        allKindGraph->showNumericalResults(show);
    
    for (GraphViewResults* allKindGraph : mByEventsGraphs)
        allKindGraph->showNumericalResults(show);
    
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

            QFile file(dirPath + "/Model_description.html");
            if (file.open(QFile::WriteOnly | QFile::Truncate)) {
                QTextStream output(&file);
                output<<version+"<br>";
                output<<projectName+ "<br>";
                output<<"<hr>";
                output<<mModel->getModelLog();
            }
            file.close();

            file.setFileName(dirPath + "/MCMC_Initialization.html");
            
            if (file.open(QFile::WriteOnly | QFile::Truncate)) {
                QTextStream output(&file);
                output<<version+"<br>";
                output<<projectName+ "<br>";
                output<<"<hr>";
                output<<mModel->getMCMCLog();
            }
            file.close();

            file.setFileName(dirPath + "/Posterior_distrib_results.html");
            
            if (file.open(QFile::WriteOnly | QFile::Truncate)) {
                QTextStream output(&file);
                output<<version+"<br>";
                output<<projectName+ "<br>";
                output<<"<hr>";
                output<<mModel->getResultsLog();
            }
            file.close();

            const QList<QStringList> stats = mModel->getStats(csvLocal, precision, true);
            saveCsvTo(stats, dirPath + "/Stats_table.csv", csvSep, true);
            
            if (mModel->mPhases.size() > 0) {
                const QList<QStringList> phasesTraces = mModel->getPhasesTraces(csvLocal, false);
                saveCsvTo(phasesTraces, dirPath + "/phases.csv", csvSep, false);
                
                for (int i=0; i<mModel->mPhases.size(); ++i) {
                    const QList<QStringList> phaseTrace = mModel->getPhaseTrace(i,csvLocal, false);
                    const QString name = mModel->mPhases.at(i)->mName.toLower().simplified().replace(" ", "_");
                    saveCsvTo(phaseTrace, dirPath + "/phase_" + name + ".csv", csvSep, false);
                }
            }
            QList<QStringList> eventsTraces = mModel->getEventsTraces(csvLocal, false);
            saveCsvTo(eventsTraces, dirPath + "/events.csv", csvSep, false);
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
    
    //ScrollArrea witchScroll;
    bool printAxis = true;
    
    QWidget* curWid;
    
    if (mStack->currentWidget() == mPhasesScrollArea) {
        curWid = mPhasesScrollArea->widget();
        curWid->setFont(mByPhasesGraphs.at(0)->font());
       // witchScroll = eScrollPhases;
        //  hide all buttons in the both scrollAreaWidget
        for (int i=0; i<mByPhasesGraphs.size(); ++i)
            mByPhasesGraphs.at(i)->setButtonsVisible(false);
        
    }
    
    else  {
        curWid = mEventsScrollArea->widget();
        curWid->setFont(mByEventsGraphs.at(0)->font());
        //witchScroll = eScrollEvents;
        //  hide all buttons in the both scrollAreaWidget
        for (int i=0; i<mByEventsGraphs.size(); ++i)
            mByEventsGraphs.at(i)->setButtonsVisible(false);
        
    }
    
    
    // --------------------------------------------------------------------
    // Force rendering to HD for export
    int rendering = mRenderCombo->currentIndex();
    updateRendering(1);
    
    AxisWidget* axisWidget = nullptr;
    QLabel* axisLegend = nullptr;
    int axeHeight = 20;
    int legendHeight = 20;
    
    if (printAxis) {
        curWid->setFixedHeight(curWid->height() + axeHeight + legendHeight);
        
        FormatFunc f = 0;
        if (mTabs->currentIndex() == 0 && mDataThetaRadio->isChecked())
            f = stringWithAppSettings;
        
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
        
        axisLegend = new QLabel(DateUtils::getAppSettingsFormatStr(), curWid);
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
    
    //  show all buttons
    if (mByPhasesBut->isChecked())
        for (int i=0; i<mByPhasesGraphs.size(); ++i)
            mByPhasesGraphs.at(i)->setButtonsVisible(true);
    
    else
        for (int i=0; i<mByEventsGraphs.size(); ++i)
            mByEventsGraphs.at(i)->setButtonsVisible(true);
    
}

//#pragma mark Refresh All Model
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
        while (iterEvent != mModel->mEvents.cend()) {

            if ((*iterEvent)->mId == eventId) {
                (*iterEvent)->mName  = eventJSON.value(STATE_NAME).toString();
                (*iterEvent)->mItemX = eventJSON.value(STATE_ITEM_X).toDouble();
                (*iterEvent)->mItemY = eventJSON.value(STATE_ITEM_Y).toDouble();
                (*iterEvent)->mColor = QColor(eventJSON.value(STATE_COLOR_RED).toInt(),
                                              eventJSON.value(STATE_COLOR_GREEN).toInt(),
                                              eventJSON.value(STATE_COLOR_BLUE).toInt());
               
                
                for (int k=0; k<(*iterEvent)->mDates.size(); ++k) {
                    Date& d = (*iterEvent)->mDates[k];
                    
                    foreach (const QJsonValue dateVal, dates) {

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
        
        for ( auto&& p : mModel->mPhases ) {
            if (p->mId == phaseId) {
                p->mName = phaseJSON.value(STATE_NAME).toString();
                p->mItemX = phaseJSON.value(STATE_ITEM_X).toDouble();
                p->mItemY = phaseJSON.value(STATE_ITEM_Y).toDouble();
                p->mColor = QColor(phaseJSON.value(STATE_COLOR_RED).toInt(),
                                   phaseJSON.value(STATE_COLOR_GREEN).toInt(),
                                   phaseJSON.value(STATE_COLOR_BLUE).toInt());
                break;
            }
        }
        ++iterJSONPhase;
    }
    
    std::sort(mModel->mEvents.begin(), mModel->mEvents.end(), sortEvents);
    std::sort(mModel->mPhases.begin(), mModel->mPhases.end(), sortPhases);
    
    updateResults(mModel);
}

void ResultsView::adjustDuration(bool visible)
{
    qreal durationMax = 0.;
    // find the durationMax
    if (mByPhasesBut->isChecked()) {
        for (const GraphViewResults* allGraphs : mByPhasesGraphs) {
            const GraphViewPhase* phaseGraph = dynamic_cast<const GraphViewPhase*>(allGraphs);
            if (phaseGraph) {
                GraphView *durationGraph =phaseGraph->mDurationGraph;

                if (durationGraph && durationGraph->isVisible() ) {
                    GraphCurve *durationCurve = durationGraph->getCurve("Duration");
                    if (durationCurve && !durationCurve->mData.isEmpty()){
                       durationMax = qMax((qreal)durationCurve->mData.lastKey(), durationMax);
                    }
                 }
             }
        }
qDebug()<<"ResultsView::adjustDuration "<<durationMax;
        
    // set the same RangeX with durationMax

       for (const GraphViewResults* allGraphs : mByPhasesGraphs) {
            const GraphViewPhase* phaseGraph = dynamic_cast<const GraphViewPhase*>(allGraphs);
            if (phaseGraph) {
                GraphView *durationGraph =phaseGraph->mDurationGraph;

                if (durationGraph) {
                    GraphCurve *durationCurve = durationGraph->getCurve("Duration");
                    if (durationCurve && !durationCurve->mData.isEmpty()){
                       durationGraph->setRangeX(0, durationMax);
                       durationGraph->setCurrentX(0, durationMax);
                       durationGraph->zoomX(0, durationMax);
                    }
                 }
             }

        }
    }

}

