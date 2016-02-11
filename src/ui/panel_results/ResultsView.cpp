
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
    connect(mFontBut, &QPushButton::clicked, this, &ResultsView::updateFont);
    
    mThicknessSpin = new QSpinBox();
    mThicknessSpin->setRange(1, 10);
    mThicknessSpin->setSuffix(" px");
    connect(mThicknessSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ResultsView::updateThickness);
    
    mOpacitySpin = new QSpinBox();
    mOpacitySpin->setRange(0, 100);
    mOpacitySpin->setValue(30);
    mOpacitySpin->setSuffix(" %");
    connect(mOpacitySpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ResultsView::updateOpacity);


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
    mThreshLab = new Label(tr("HPD / Credibility (%)") + " :", mPostDistGroup);
    mThreshLab->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    
    mHPDEdit = new LineEdit(mPostDistGroup);
    mHPDEdit->QWidget::setStyleSheet("QLineEdit { border-radius: 5px; }");
    mHPDEdit->setText("95");
    
    DoubleValidator* percentValidator = new DoubleValidator();
    percentValidator->setBottom(0.);
    percentValidator->setTop(100.);
    percentValidator->setDecimals(1);
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
    
    connect(this, &ResultsView::posteriorDistribGenerated, this, &ResultsView::generateCredibilityAndHPD);
    connect(this, &ResultsView::credibilityAndHPDGenerated, this, &ResultsView::generateCurves);
    connect(this, &ResultsView::curvesGenerated, this, &ResultsView::updateCurvesToShow);
    
    connect(this, &ResultsView::controlsUpdated, this, &ResultsView::updateLayout);
    
    // -------------------------
    
    connect(mTabs, &Tabs::tabClicked, this, &ResultsView::updateControls);
    connect(mByPhasesBut, &Button::clicked, this, &ResultsView::updateControls);
    connect(mByEventsBut, &Button::clicked, this, &ResultsView::updateControls);
    
    connect(mTabs, &Tabs::tabClicked, this, &ResultsView::generateCurves);
    connect(mByPhasesBut, &Button::clicked, this, &ResultsView::generateCurves);
    connect(mByEventsBut, &Button::clicked, this, &ResultsView::generateCurves);
    
    // -------------------------
    
    connect(mFFTLenCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ResultsView::generatePosteriorDistribs);
    connect(mHFactorEdit, &LineEdit::editingFinished, this, &ResultsView::generatePosteriorDistribs);
    
    connect(mHPDEdit, &LineEdit::editingFinished, this, &ResultsView::generateCredibilityAndHPD);
    
    connect(mDataThetaRadio, &RadioButton::clicked, this, &ResultsView::generateCurves);
    connect(mDataSigmaRadio, &RadioButton::clicked, this, &ResultsView::generateCurves);
    
    connect(mDataCalibCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mWiggleCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mAllChainsCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    connect(mCredibilityCheck, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
    
    // -------------------------
    //connect(mUnfoldBut, &Button::toggled, this, &ResultsView::createEventsScrollArea);
    connect(mUnfoldBut, &Button::toggled, this, &ResultsView::updateGraphsLayout);
    connect(mShowDataUnderPhasesCheck, &CheckBox::toggled, this, &ResultsView::updateGraphsLayout);
    
    // -------------------------


    connect(mXSlider, &QSlider::sliderMoved, this, &ResultsView::updateZoomX);
    connect(mXSlider, &QSlider::sliderPressed, this, &ResultsView::updateZoomX);
    connect(mCurrentXMinEdit, &LineEdit::editingFinished, this, &ResultsView::editCurrentMinX);
    connect(mCurrentXMaxEdit, &LineEdit::editingFinished, this, &ResultsView::editCurrentMaxX);
    connect(mRuler, &Ruler::positionChanged, this, &ResultsView::updateScroll);
    
    connect(mYSlider, &QSlider::valueChanged, this, &ResultsView::updateScaleY);
    
    // -------------------------
    
    connect(mRenderCombo,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ResultsView::updateRendering);
    connect(mInfosBut, &Button::toggled, this, &ResultsView::showInfos);
    connect(mExportImgBut, &Button::clicked, this, &ResultsView::exportFullImage);
    connect(mExportResults, &Button::clicked, this, &ResultsView::exportResults);
    
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
    connect(project, &Project::mcmcStarted, this, &ResultsView::clearResults);
    
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
        QRect rect = graphs.at(i)->geometry();
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
            mCheckChainChecks.at(i)->setVisible(true);
        
        for(int i=0; i<mChainRadios.size(); ++i)
            mChainRadios.at(i)->setVisible(false);
    }
    else
    {
        for(int i=0; i<mCheckChainChecks.size(); ++i)
            mCheckChainChecks.at(i)->setVisible(false);
        
        for(int i=0; i<mChainRadios.size(); ++i)
            mChainRadios.at(i)->setVisible(true);
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
    int dx = mLineH + mMargin;
    
    int tabIdx = mTabs->currentIndex();
    
    mCurrentXMinEdit->setText( DateUtils::dateToString(mResultCurrentMinX) );
    mCurrentXMaxEdit->setText( DateUtils::dateToString(mResultCurrentMaxX) );
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
            mCheckChainChecks.at(i)->setGeometry(geometry);
            mChainRadios.at(i)->setGeometry(geometry);
        }
    }
    // trace, accept or correl : chains are selectable with radio-buttons
    else
    {
        mChainsGroup->setFixedHeight(m + numChains * (mLineH + m));
        for(int i=0; i<numChains; ++i)
        {
            QRect geometry(m, (int)(m + i * (mLineH + m)), (int)(mChainsGroup->width()-2*m), mLineH);
            mCheckChainChecks.at(i) -> setGeometry(geometry);
            mChainRadios.at(i)     -> setGeometry(geometry);
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

    if(mByPhasesBut->isChecked()) {
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
        mByEventsGraphs.at(i)->setGeometry(geometries.at(i));
        h += geometries.at(i).height();
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
        mByPhasesGraphs.at(i)->setGeometry(geometries.at(i));
        h += geometries.at(i).height();
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
        disconnect(check, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
        check->setParent(0);
        delete check;
    }
    mCheckChainChecks.clear();
    
    for(int i=mChainRadios.size()-1; i>=0; --i)
    {
        RadioButton* but = mChainRadios.takeAt(i);
        disconnect(but, &RadioButton::clicked, this, &ResultsView::updateCurvesToShow);
        but->setParent(0);
        delete but;
    }
    mChainRadios.clear();
    
    for(int i=0; i<mByEventsGraphs.size(); ++i)
    {
        mByEventsGraphs.at(i)->setParent(0);
        delete mByEventsGraphs[i];
    }
    mByEventsGraphs.clear();
    
    for(int i=0; i<mByPhasesGraphs.size(); ++i)
    {
        mByPhasesGraphs.at(i)->setParent(0);
        delete mByPhasesGraphs[i];
    }
    mByPhasesGraphs.clear();
    
    QWidget* eventsWidget = mEventsScrollArea->takeWidget();
    if(eventsWidget)
        delete eventsWidget;
    
    QWidget* phasesWidget = mPhasesScrollArea->takeWidget();
    if(phasesWidget)
        delete phasesWidget;

    mResultMinX = mSettings.getTminFormated();
    mResultMaxX = mSettings.getTmaxFormated();
    mResultCurrentMinX = mResultMinX ;
    mResultCurrentMaxX = mResultMaxX ;
    mResultZoomX = 1;
    updateZoomEdit();
}


void ResultsView::updateFormatSetting(Model* model, const AppSettings* appSet)
{
    if(!mModel && !model)
        return;
    if(model)
        mModel = model;
    mModel->updateFormatSettings(appSet);
}


/**
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
    if(mCheckChainChecks.isEmpty()) {
        for(int i=0; i<mChains.size(); ++i)
        {
            CheckBox* check = new CheckBox(tr("Chain") + " " + QString::number(i+1), mChainsGroup);
            connect(check, &CheckBox::clicked, this, &ResultsView::updateCurvesToShow);
            check->setVisible(true);
            mCheckChainChecks.append(check);

            RadioButton* radio = new RadioButton(tr("Chain") + " " + QString::number(i+1), mChainsGroup);
            connect(radio, &RadioButton::clicked, this, &ResultsView::updateCurvesToShow);
            radio->setVisible(true);
            if(i == 0)
                radio->setChecked(true);
            mChainRadios.append(radio);
        }
    }

    // ----------------------------------------------------
    //  Events Views : generate all phases graph
    //  No posterior density has been computed yet!
    //  Graphs are empty at the moment
    // ----------------------------------------------------

    createEventsScrollArea();

    // ----------------------------------------------------
    //  Phases Views : generate all phases graph
    //  No posterior density has been computed yet!
    //  Graphs are empty at the moment
    // ----------------------------------------------------

    createPhasesScrollArea();


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

    QList<Event*>::const_iterator iterEvent = mModel->mEvents.cbegin();
    while (iterEvent!=mModel->mEvents.cend()) {

        GraphViewEvent* graphEvent = new GraphViewEvent(eventsWidget);

        // ----------------------------------------------------
        //  This just creates the view for the event.
        //  It sets the Event which triggers an update() to repaint the view.
        //  The refresh() function which actually creates the graph curves will be called later.
        // ----------------------------------------------------
        graphEvent->setSettings(mModel->mSettings);
        graphEvent->setMCMCSettings(mModel->mMCMCSettings, mChains);
        graphEvent->setEvent((*iterEvent));
        graphEvent->setGraphFont(mFont);
        graphEvent->setGraphsThickness(mThicknessSpin->value());
        mByEventsGraphs.append(graphEvent);

        if((*iterEvent)->mType != Event::eKnown) {
             for(int j=0; j<(*iterEvent)->mDates.size(); ++j) {
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
       }
       ++iterEvent;
    }
    mEventsScrollArea->setWidget(eventsWidget);
}

void ResultsView::createPhasesScrollArea()
{
    QWidget* phasesWidget = new QWidget();
    phasesWidget->setMouseTracking(true);


        // ----------------------------------------------------
        //  This just creates the GraphView for the phase (no curve yet)
        // ----------------------------------------------------

    QList<Phase*>::const_iterator iterPhase = mModel->mPhases.cbegin();
    while (iterPhase!=mModel->mPhases.cend()) {

        GraphViewPhase* graphPhase = new GraphViewPhase(phasesWidget);

        graphPhase->setSettings(mModel->mSettings);
        graphPhase->setMCMCSettings(mModel->mMCMCSettings, mChains);
        graphPhase->setPhase((*iterPhase));
        graphPhase->setGraphFont(mFont);
        graphPhase->setGraphsThickness(mThicknessSpin->value());
        mByPhasesGraphs.append(graphPhase);

        // ----------------------------------------------------
        //  This just creates the GraphView for the event (no curve yet)
        // ----------------------------------------------------

        QList<Event*>::iterator iterEvent = (*iterPhase)->mEvents.begin();
        while (iterEvent!=(*iterPhase)->mEvents.end()) {

            GraphViewEvent* graphEvent = new GraphViewEvent(phasesWidget);
            graphEvent->setSettings(mModel->mSettings);
            graphEvent->setMCMCSettings(mModel->mMCMCSettings, mChains);
            graphEvent->setEvent((*iterEvent));
            graphEvent->setGraphFont(mFont);
            graphEvent->setGraphsThickness(mThicknessSpin->value());
            mByPhasesGraphs.append(graphEvent);

            // --------------------------------------------------
            //  This just creates the GraphView for the date (no curve yet)
            // --------------------------------------------------
            for(int j=0; j<(*iterEvent)->mDates.size(); ++j)
            {
                Date& date = (*iterEvent)->mDates[j];
                GraphViewDate* graphDate = new GraphViewDate(phasesWidget);
                graphDate->setSettings(mModel->mSettings);
                graphDate->setMCMCSettings(mModel->mMCMCSettings, mChains);
                graphDate->setDate(&date);
                graphDate->setColor((*iterEvent)->mColor);
                graphDate->setGraphFont(mFont);
                graphDate->setGraphsThickness(mThicknessSpin->value());

                mByPhasesGraphs.append(graphDate);
            }
            ++iterEvent;
        }
        ++iterPhase;
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
        const int len = mFFTLenCombo->currentText().toInt();
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
        const QLocale locale;
        bool ok;
        QString input = mHPDEdit->text();
        mHPDEdit->validator()->fixup(input);
        mHPDEdit->setText(input);
        
        double hpd = locale.toDouble(mHPDEdit->text(),&ok);
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

        QList<GraphViewResults*>::const_iterator constIter;
        if (mByPhasesBut->isChecked()) {
            constIter = mByPhasesGraphs.cbegin();
            iterEnd = mByPhasesGraphs.cend();
        }
        else {
            constIter = mByEventsGraphs.cbegin();
            iterEnd = mByEventsGraphs.cend();
        }

        while(constIter != iterEnd) {
            const GraphViewDate* graphDate = dynamic_cast<const GraphViewDate*>(*constIter);
            if(graphDate) {
               const GraphCurve* graphVariance = graphDate->mGraph->getCurve("Post Distrib All Chains");
               if(graphVariance) {
                   mResultMaxVariance = ceil(qMax(mResultMaxVariance, graphVariance->mData.lastKey()));
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
    
    const ProjectSettings s = mSettings;
    const MCMCSettings mcmc = mMCMCSettings;
    
    const bool showAllChains = mAllChainsCheck->isChecked();
    QList<bool> showChainList;
    if(mTabs->currentIndex() == 0)
    {
        foreach (CheckBox* cbButton, mCheckChainChecks) {
            showChainList.append(cbButton->isChecked());
        }
    }
    else
    {
        foreach (RadioButton* rButton, mChainRadios) {
            showChainList.append(rButton->isChecked());
        }
    }
    const bool showCalib = mDataCalibCheck->isChecked();
    const bool showWiggle = mWiggleCheck->isChecked();
    const bool showCredibility = mCredibilityCheck->isChecked();
    

    foreach (GraphViewResults* phaseGraph, mByPhasesGraphs) {
         phaseGraph->updateCurvesToShow(showAllChains, showChainList, showCredibility, showCalib, showWiggle);
    }

    foreach (GraphViewResults* eventGraph, mByEventsGraphs) {
         eventGraph->updateCurvesToShow(showAllChains, showChainList, showCredibility, showCalib, showWiggle);
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
            mResultMinX = s.getTminFormated();
            mResultMaxX = s.getTmaxFormated();
        }
        else if(mDataSigmaRadio->isChecked())  {
            mResultMinX = 0;
            mResultMaxX = mResultMaxVariance;

        }
    }
    else if(tabIdx == 1 || tabIdx == 2){
        mResultMinX = 0;
        for(int i=0; i<mChainRadios.size(); ++i){
            if(mChainRadios.at(i)->isChecked()){
                const ChainSpecs& chain = mChains.at(i);
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
        mResultCurrentMinX = mZooms.value(tabIdx).first;
        mResultCurrentMaxX = mZooms.value(tabIdx).second;
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

    foreach (GraphViewResults* phaseGraph, mByPhasesGraphs) {
        phaseGraph->setRange(mResultMinX, mResultMaxX);
        phaseGraph->setCurrentX(mResultCurrentMinX, mResultCurrentMaxX);
    }

    foreach (GraphViewResults* eventGraph, mByEventsGraphs) {

        eventGraph->setRange(mResultMinX, mResultMaxX);
        eventGraph->setCurrentX(mResultCurrentMinX, mResultCurrentMaxX);
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
    mRuler->setFormatFunctX(formatValueToAppSettingsPrecision);

    mRuler->setRange(mResultMinX, mResultMaxX);
    mRuler->setCurrent(mResultCurrentMinX, mResultCurrentMaxX);
    
    // ------------------------------------------
    //  Set Ruler Areas (Burn, Adapt, Run)
    // ------------------------------------------
    mRuler->clearAreas();
    if(tabIdx == 1 || tabIdx == 2){
        for(int i=0; i<mChainRadios.size(); ++i)
        {
            if(mChainRadios.at(i)->isChecked()){
                const ChainSpecs& chain = mChains.at(i);
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
        log += ModelUtilities::eventResultsHTML(event, true, mModel);
    }
    for(int i=0; i<mModel->mPhases.size(); ++i)
    {
        Phase* phase = mModel->mPhases[i];
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
   /* if(mTabs->currentIndex() == 0 && mDataThetaRadio->isChecked()){
        mCurrentXMinEdit->setText(DateUtils::convertToAppSettingsFormatStr(mResultCurrentMinX));
        mCurrentXMaxEdit->setText(DateUtils::convertToAppSettingsFormatStr(mResultCurrentMaxX));
    }else{
        mCurrentXMinEdit->setText(QString::number(mResultCurrentMinX));
        mCurrentXMaxEdit->setText(QString::number(mResultCurrentMaxX));
    }*/
    mCurrentXMinEdit->setText(DateUtils::dateToString(mResultCurrentMinX));
    mCurrentXMaxEdit->setText(DateUtils::dateToString(mResultCurrentMaxX));
}

void ResultsView::updateGraphsZoomX()
{
    foreach (GraphViewResults* phaseGraph, mByPhasesGraphs) {
        phaseGraph->zoom(mResultCurrentMinX, mResultCurrentMaxX);
    }
    foreach (GraphViewResults* eventGraph, mByEventsGraphs) {
        eventGraph->zoom(mResultCurrentMinX, mResultCurrentMaxX);
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
    const double min = 70;
    const double max = 1070;
    const double prop = value / 100.f;
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
        
        foreach (GraphViewResults* phaseGraph, mByPhasesGraphs) {
            phaseGraph->setGraphFont(mFont);
        }
        mPhasesScrollArea->setFont(mFont);

        foreach (GraphViewResults* eventGraph, mByEventsGraphs) {
            eventGraph->setGraphFont(mFont);
        }
        mEventsScrollArea->setFont(mFont);

    }
}

void ResultsView::updateThickness(int value)
{
    foreach (GraphViewResults* allKindGraph, mByPhasesGraphs) {
        allKindGraph->setGraphsThickness(value);
        GraphViewPhase* phaseGraphs = dynamic_cast<GraphViewPhase*>(allKindGraph);
        if(phaseGraphs) {
            phaseGraphs->mDurationGraph->setGraphsThickness(value);
        }
    }

    foreach (GraphViewResults* allKindGraph, mByEventsGraphs) {
        allKindGraph->setGraphsThickness(value);
    }
}

void ResultsView::updateOpacity(int value)
{
    foreach (GraphViewResults* allKindGraph, mByPhasesGraphs) {
       allKindGraph->setGraphsOpacity(value);
       GraphViewPhase* phaseGraphs = dynamic_cast<GraphViewPhase*>(allKindGraph);
       if(phaseGraphs) {
           phaseGraphs->mDurationGraph->setCurvesOpacity(value);
       }
    }
    foreach (GraphViewResults* allKindGraph, mByEventsGraphs) {
        allKindGraph->setGraphsOpacity(value);
    }
}

void ResultsView::updateRendering(int index)
{
    foreach (GraphViewResults* allKindGraph, mByPhasesGraphs) {
        allKindGraph->setRendering((GraphView::Rendering) index);
        GraphViewPhase* phaseGraphs = dynamic_cast<GraphViewPhase*>(allKindGraph);
        if(phaseGraphs) {
            phaseGraphs->mDurationGraph->setRendering((GraphView::Rendering) index);
        }
    }
    foreach (GraphViewResults* allKindGraph, mByEventsGraphs) {
        allKindGraph->setRendering((GraphView::Rendering) index);
    }
}

void ResultsView::showInfos(bool show)
{
    foreach (GraphViewResults* allKindGraph, mByPhasesGraphs) {
        allKindGraph->showNumericalResults(show);
    }
    foreach (GraphViewResults* allKindGraph, mByEventsGraphs) {
        allKindGraph->showNumericalResults(show);
    }
}

void ResultsView::exportResults()
{
    if(mModel){
        AppSettings settings = MainWindow::getInstance()->getAppSettings();
        const QString csvSep = settings.mCSVCellSeparator;
        
        QLocale csvLocal = settings.mCSVDecSeparator == "." ? QLocale::English : QLocale::French;

        csvLocal.setNumberOptions(QLocale::OmitGroupSeparator);
        
        const QString currentPath = MainWindow::getInstance()->getCurrentPath();
        const QString dirPath = QFileDialog::getSaveFileName(qApp->activeWindow(),
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

            // copy tabs ------------------------------------------
            const QString version = qApp->applicationName() + " " + qApp->applicationVersion();
            const QString projectName = tr("Project filename")+" : "+ MainWindow::getInstance()->getNameProject()+ "<br>";

            QFile file(dirPath + "/Model_description.html");
            if(file.open(QFile::WriteOnly | QFile::Truncate))
            {
                QTextStream output(&file);
                output<<version+"<br>";
                output<<projectName+ "<br>";
                output<<"<hr>";
                output<<mModel->getModelLog();
            }
            file.close();

            file.setFileName(dirPath + "/MCMC_Initialization.html");
            if(file.open(QFile::WriteOnly | QFile::Truncate))
            {
                QTextStream output(&file);
                output<<version+"<br>";
                output<<projectName+ "<br>";
                output<<"<hr>";
                output<<mModel->getMCMCLog();;
            }
            file.close();

            file.setFileName(dirPath + "/Posterior_distrib_results.html");
            if(file.open(QFile::WriteOnly | QFile::Truncate))
            {
                QTextStream output(&file);
                output<<version+"<br>";
                output<<projectName+ "<br>";
                output<<"<hr>";
                output<<mModel->getResultsLog();;
            }
            file.close();

            const QList<QStringList> stats = mModel->getStats(csvLocal, true);
            saveCsvTo(stats, dirPath + "/Stats_table.csv", csvSep, true);
            
            if(mModel->mPhases.size() > 0){
                const QList<QStringList> phasesTraces = mModel->getPhasesTraces(csvLocal, false);
                saveCsvTo(phasesTraces, dirPath + "/phases.csv", csvSep, false);
                
                for(int i=0; i<mModel->mPhases.size(); ++i){
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
    
    ScrollArrea witchScroll;
    bool printAxis = true;
    
    QWidget* curWid;
    
    if (mStack->currentWidget() == mPhasesScrollArea) {
        curWid = mPhasesScrollArea->widget();
        curWid->setFont(mByPhasesGraphs.at(0)->font());
        witchScroll = eScrollPhases;
        //  hide all buttons in the both scrollAreaWidget
        for(int i=0; i<mByPhasesGraphs.size(); ++i){
            mByPhasesGraphs.at(i)->setButtonsVisible(false);
        }
    }
    //else if (mStack->currentWidget() == mEventsScrollArea) {
    else  {
        curWid = mEventsScrollArea->widget();
        curWid->setFont(mByEventsGraphs.at(0)->font());
        witchScroll = eScrollEvents;
        //  hide all buttons in the both scrollAreaWidget
        for(int i=0; i<mByEventsGraphs.size(); ++i) {
            mByEventsGraphs.at(i)->setButtonsVisible(false);
        }
    }
    
    
    // --------------------------------------------------------------------
    // Force rendering to HD for export
    int rendering = mRenderCombo->currentIndex();
    updateRendering(1);
    
    AxisWidget* axisWidget = 0;
    QLabel* axisLegend = 0;
    int axeHeight = 20;
    int legendHeight = 20;
    
    if (printAxis) {
        curWid->setFixedHeight(curWid->height() + axeHeight + legendHeight);
        
        FormatFunc f = 0;
        if(mTabs->currentIndex() == 0 && mDataThetaRadio->isChecked())
            f = formatValueToAppSettingsPrecision;
        
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
            mByPhasesGraphs.at(i)->setButtonsVisible(true);
        }
        
    }
    else {
        for(int i=0; i<mByEventsGraphs.size(); ++i) {
            mByEventsGraphs.at(i)->setButtonsVisible(true);
        }
    }
}

#pragma mark Refresh All Model
/**
 * @brief ResultsView::updateModel Update Design
 */
void ResultsView::updateModel()
{
    if(!mModel)
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

            if((*iterEvent)->mId == eventId)
            {
                (*iterEvent)->mName  = eventJSON.value(STATE_NAME).toString();
                (*iterEvent)->mItemX = eventJSON.value(STATE_ITEM_X).toDouble();
                (*iterEvent)->mItemY = eventJSON.value(STATE_ITEM_Y).toDouble();
                (*iterEvent)->mColor = QColor(eventJSON.value(STATE_COLOR_RED).toInt(),
                                              eventJSON.value(STATE_COLOR_GREEN).toInt(),
                                              eventJSON.value(STATE_COLOR_BLUE).toInt());
               
                
                for(int k=0; k<(*iterEvent)->mDates.size(); ++k)
                {
                    Date& d = (*iterEvent)->mDates[k];
                    
                    foreach (const QJsonValue dateVal, dates) {

                        const QJsonObject date = dateVal.toObject();
                        const int dateId = date.value(STATE_ID).toInt();
                        
                        if(dateId == d.mId)
                        {
                            d.mName = date.value(STATE_NAME).toString();

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
        
        for(int j=0; j<mModel->mPhases.size(); ++j)
        {
            Phase* p = mModel->mPhases[j];
            if(p->mId == phaseId)
            {
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
    int durationMax = 0;
    // find the durationMax
    foreach (const GraphViewResults* allGraphs, mByPhasesGraphs) {
        const GraphViewPhase* phaseGraph = dynamic_cast<const GraphViewPhase*>(allGraphs);
        if(phaseGraph) {
            GraphView *durationGraph =phaseGraph->mDurationGraph;

            if(durationGraph && durationGraph->isVisible() ) {
                GraphCurve *durationCurve = durationGraph->getCurve("Duration");
                if(durationCurve && !durationCurve->mData.isEmpty()){
                   durationMax = qMax((int)durationCurve->mData.lastKey(),durationMax);
                }
             }
         }
    }
    // set the same RangeX with durationMax
   foreach (const GraphViewResults* allGraphs, mByPhasesGraphs) {
        const GraphViewPhase* phaseGraph = dynamic_cast<const GraphViewPhase*>(allGraphs);
        if(phaseGraph) {
            GraphView *durationGraph =phaseGraph->mDurationGraph;
            if(durationGraph) {
                GraphCurve *durationCurve = durationGraph->getCurve("Duration");
                if(durationCurve && !durationCurve->mData.isEmpty()){
                   durationGraph->setRangeX(0, durationMax);
                   durationGraph->setCurrentX(0, durationMax);
                }
             }
         }

    }

}
