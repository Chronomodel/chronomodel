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

#ifndef RESULTSWRAPPER_H
#define RESULTSWRAPPER_H

#include "GraphViewResults.h"

#include <QVBoxLayout>
#include <QTabWidget>
#include <QLabel>
#include <QLineEdit>
#include <qapplication.h>
#include <QStyle>

class QStackedWidget;
class QScrollArea;
class QTimer;
class QComboBox;
class QSlider;
class QScrollBar;
class QSpinBox;
class QDoubleSpinBox;
class QPushButton;

class Project;
class Model;
class ModelCurve;
class Tabs;
class Ruler;
class GraphView;
class GraphViewPhase;
class GraphViewEvent;
class GraphViewDate;

class Label;
class Button;
class LineEdit;
class CheckBox;
class RadioButton;
class Marker;

class ResultsView: public QWidget
{
    Q_OBJECT
public:
    //ResultsView(std::shared_ptr<Project> project, QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Widget);
    ResultsView( QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Widget);
    ~ResultsView();

   /* void setProject(const std::shared_ptr<Project> project);
    void initModel(const std::shared_ptr<ModelCurve> model);
    */
    void setProject();
    void initModel();
    void updateModel();
    void applyAppSettings();

protected:
    void updateEventsOptions(qreal &optionWidgetHeight, bool isPostDistrib);
    void updatePhasesOptions(qreal& optionWidgetHeight);
    void updateCurvesOptions(qreal& optionWidgetHeight);
    void updateDisplayOptions(qreal& optionWidgetHeight);
    void updateDistribOptions(qreal& optionWidgetHeight, bool isPostDistrib);
    void updatePageSaveOptions(qreal& optionWidgetHeight);
    void updateOptionsWidget();
    // ------------------------------------------------
    //  Events & Layout
    // ------------------------------------------------
    virtual bool event(QEvent *e);
    void mouseMoveEvent(QMouseEvent* e);
    void resizeEvent(QResizeEvent* e);

    void updateMarkerGeometry(const int x);
    void updateGraphsLayout();
    void updateGraphsLayout(QScrollArea* scrollArea, QList<GraphViewResults*> graphs);

    // ------------------------------------------------
    //  Graphs UI
    // ------------------------------------------------

    void createByEventsGraphs();
    void createByPhasesGraphs();

    void createByCurveGraph();
    void createByAlphaGraph();
    
    void deleteAllGraphsInList(QList<GraphViewResults*>& list);
    QList<GraphViewResults*> allGraphs();
    QList<GraphViewResults*> currentGraphs(bool onlySelected);
    bool hasSelectedGraphs();
    
    void updateGraphsMinMax();
    double getGraphsMax(const QList<GraphViewResults*>& graphs, const QString& title, double maxFloor = -INFINITY);
    double getGraphsMin(const QList<GraphViewResults*>& graphs, const QString& title, double minCeil = INFINITY);

    
    // ------------------------------------
    //  Pagination
    // ------------------------------------
    bool graphIndexIsInCurrentPage(int graphIndex);

    // ------------------------------------------------
    //  Chains controls
    // ------------------------------------------------
    void createChainsControls();
    void deleteChainsControls();

    // ------------------------------------------------
    //  Span options
    // ------------------------------------------------
    GraphViewResults::variable_t getMainVariable() const;
    void setTimeRange();
    void setTimeSlider(const int value);
    void setTimeEdit(const double value);
    void setTimeScale();

    // ------------------------------------------------
    //  Utilities
    // ------------------------------------------------
    inline bool isPostDistribGraph()
    {
        return (mCurrentTypeGraph == GraphViewResults::ePostDistrib);
    }

    inline bool xScaleRepresentsTime();
    inline double sliderToZoom(const int coef);
    inline int zoomToSlider(const double &zoom);

    // ------------------------------------------------
    //  Controls actions helpers
    // ------------------------------------------------
    void updateZoomT();
    void updateGraphsZoomT();
    void updateGraphsHeight();

    // ------------------------------------------------
    //  Curve
    // ------------------------------------------------

    inline bool isCurve();
    void createOptionsWidget();

public slots:

    void clearResults(); // connected to Project::mcmcStarted
    void generateCurves();

private slots:

    // ------------------------------------------------
    //  Layout
    // ------------------------------------------------
    void updateLayout();
    void showStats(bool);

    // ------------------------------------------------
    //  Graphs / Curves / Controls
    // ------------------------------------------------
    void updateMainVariable();
    void createGraphs();


    void updateCurvesToShow();
    /**
     *  @brief
     *  This method does the following :
     *  - Defines [mResultMinT, mResultMaxT]
     *  - Defines [mResultCurrentMinT, mResultCurrentMaxT] (based on saved zoom if any)
     *  - Computes mResultZoomT
     *  - Set Ruler Areas
     *  - Set Ruler and graphs range with mZoomsT
     *  - Update mXMinEdit, mXMaxEdit, mXSlider, mTimeSpin, mMajorScaleEdit, mMinorScaleEdit
     *  - Set slider and  zoomEdit with mZoomsH
     */
    //void extracted(int &idSelect);
    void updateScales();
    void updateTotalGraphs();
    
    // ------------------------------------
    //  Controls actions
    // ------------------------------------
    void applyRuler(const double min, const double max);

    // ------------------------------------
    //  Display / Distrib. Option
    // ------------------------------------
    void toggleDisplayDistrib();

    void applyGraphTypeTab();
    void applyGraphListTab();

    void applyCurrentVariable();


    // Span options
    void applyStudyPeriod();
    void applyTimeRange();
    void applyTimeSlider(int value);
    void applyTimeEdit();
    void applyZoomScale();

    // X, Y, Z options
    void applyXRange();
    void applyYRange();
    void applyZRange();

    void findOptimalX();
    void findOptimalY();
    void findOptimalZ();

    void setXRange();
    void setYRange();
    void setZRange();

    // Graphic options
    void applyZoomSlider(int value);
    void applyZoomEdit();
    void applyFont();
    void applyThickness(const int value);
    void applyOpacity(const int value);


    void setGraphicOption(GraphViewResults &graph);

    // Density options
    void applyFFTLength();
    void applyBandwidth();
    void applyThreshold();
    void applyHActivity();


    // ------------------------------------
    //  Page / Save
    // ------------------------------------

    // Pagination tools
    void applyGraphsPerPage(int i);
    void applyPreviousPage();
    void applyNextPage();

    // Save tools
    void exportFullImage();
    void exportResults();
    void saveAsImage();
    void imageToClipboard();
    void resultsToClipboard();
    void saveGraphData();

signals:
    void wheelMove(QEvent *e);
    void resultsLogUpdated(const QString &log);

    void xSpinUpdate(const int value);
    void xSlideUpdate(const int value);

public:
    // mModel gives access to :
    // - mModel->mSettings (StudyPeriodSettings)
    // - mModel->mMCMCSettings (MCMCSettings)
    // - mModel->mChains (QList<ChainSpecs>)


private:

    // ---------------------------------------------------------------------
    // UI useful values
    // ---------------------------------------------------------------------
    int mMargin;
    int mOptionsW;
    qreal mMarginLeft;
    qreal mMarginRight;
    qreal mGraphHeight;
    qreal mHeightForVisibleAxis;
    // The scroll bar extent (width or height depending on the orientation)
    // depends on the native platform, and must be taken into account.
    const int mSbe = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    // ---------------------------------------------------------------------
    // Left part UI components
    // ---------------------------------------------------------------------
    Tabs* mGraphTypeTabs;
    Ruler* mRuler;
    Marker* mMarker;

    QScrollArea* mEventsScrollArea;
    QScrollArea* mPhasesScrollArea;
    QScrollArea* mCurvesScrollArea;

    QWidget* mEventsWidget;
    QWidget* mPhasesWidget;
    QWidget* mCurvesWidget;

    QList<GraphViewResults*> mByEventsGraphs;
    QList<GraphViewResults*> mByPhasesGraphs;
    QList<GraphViewResults*> mByCurvesGraphs;

    QVBoxLayout* mOptionsLayout;
    // ---------------------------------------------------------------------
    // Right UI part components
    // ---------------------------------------------------------------------
    QScrollArea* mOptionsScroll;
    QWidget* mOptionsWidget;
    Tabs* mGraphListTab;

    // Tab Events
    QWidget* mEventsGroup;

    RadioButton* mEventThetaRadio;
    CheckBox* mEventsDatesUnfoldCheck;

    CheckBox* mDataCalibCheck;
    CheckBox* mWiggleCheck;
    RadioButton* mDataSigmaRadio;
#ifdef S02_BAYESIAN
    RadioButton* mS02Radio;
#endif
    RadioButton* mEventVGRadio;

    CheckBox* mEventsStatCheck;

    // Tab Phases
    QWidget* mPhasesGroup;
    RadioButton* mBeginEndRadio;
    CheckBox* mPhasesEventsUnfoldCheck;
    CheckBox* mPhasesDatesUnfoldCheck;

    RadioButton* mDurationRadio;
    RadioButton* mTempoRadio;
    RadioButton* mActivityRadio;
    CheckBox* mActivityUnifCheck;

    CheckBox* mErrCheck;
    CheckBox* mPhasesStatCheck;

    // tab Curves
    QWidget* mCurvesGroup;
    RadioButton* mCurveGRadio;
    RadioButton* mCurveGPRadio;
    RadioButton* mCurveGSRadio;
    RadioButton* mLambdaRadio;

    CheckBox* mCurveErrorCheck;
    CheckBox* mCurveHpdCheck;
    CheckBox* mCurveMapCheck;
    CheckBox* mCurveEventsPointsCheck;
    CheckBox* mCurveDataPointsCheck;
    CheckBox* mCurveStatCheck;

    // ---------------------------------------------------------------------
    // Tabs : Display / Distrib. Options
    // ---------------------------------------------------------------------
    QWidget* mDisplayGroup;
    Tabs* mDisplayDistribTab;

    //QWidget* mDisplayWidget;


    QWidget* mDistribGroup;

    // ---------------------------------------------------------------------
    //  Span options : UI components to manipulate X axis scale
    // ---------------------------------------------------------------------

    // ------------------------------------
    //  Display / Time Options
    // ------------------------------------
    QWidget* mSpanGroup;
    Label* mSpanTitle;

    // Adjust the zoom on the study period
    Button* mDisplayStudyBut;

    // Force the min T and max T
    QLabel* mSpanLab;
    LineEdit* mCurrentTMinEdit;
    LineEdit* mCurrentTMaxEdit;

    // On the X Axis scale : choose to see the whole graph at once,
    // or zoom on it adjusting the "XScale"
    QLabel* mTimeLab;
    QSlider* mTimeSlider;
    LineEdit* mTimeEdit;

    // On the X Axis scale : choose the major interval between 2 displayed values
    QLabel* mMajorScaleLab;
    LineEdit* mMajorScaleEdit;

    // On the X Axis scale : choose the number of subdivisions between 2 displyed values
    QLabel* mMinorScaleLab;
    LineEdit* mMinorScaleEdit;

    // ------------------------------------
    //  Display / X Options
    // ------------------------------------
    QWidget* mXOptionGroup;
    Label* mXOptionTitle;

    // Adjust the X scale
    Button* mXOptionBut;

    QLabel* mXOptionLab;
    LineEdit* mCurrentXMinEdit;
    LineEdit* mCurrentXMaxEdit;

    // ------------------------------------
    //  Display / Y Options
    // ------------------------------------
    QWidget* mYOptionGroup;
    Label* mYOptionTitle;

    // Adjust the X scale
    Button* mYOptionBut;

    QLabel* mYOptionLab;
    LineEdit* mCurrentYMinEdit;
    LineEdit* mCurrentYMaxEdit;

    // ------------------------------------
    //  Display / Z Options
    // ------------------------------------
    QWidget* mZOptionGroup;
    Label* mZOptionTitle;

    // Adjust the Z scale
    Button* mZOptionBut;

    QLabel* mZOptionLab;
    LineEdit* mCurrentZMinEdit;
    LineEdit* mCurrentZMaxEdit;
    // ------------------------------------
    //  Graphic Options
    // ------------------------------------
    QWidget* mGraphicGroup;
    Label* mGraphicTitle;

    QLabel* mZoomLab;
    QSlider* mZoomSlider;
    LineEdit* mZoomEdit;

    Button* mFontBut;
    QComboBox* mThicknessCombo;
    QComboBox* mOpacityCombo;

    QLabel* mLabFont;
    QLabel* mLabThickness;
    QLabel* mLabOpacity;

    // ------------------------------------
    //  MCMC Chains
    // ------------------------------------
    QWidget* mChainsGroup;
    Label* mChainsTitle;

    CheckBox* mAllChainsCheck;
    QList<CheckBox*> mChainChecks;
    QList<RadioButton*> mChainRadios;

    // ------------------------------------
    //  Density Options
    // ------------------------------------
    QWidget* mDensityOptsGroup;
    Label* mDensityOptsTitle;
    QLabel* mThreshLab;
    CheckBox* mCredibilityCheck;
    LineEdit* mThresholdEdit;

    QLabel* mFFTLenLab;
    QComboBox* mFFTLenCombo;
    QLabel* mBandwidthLab;
    LineEdit* mBandwidthEdit;
    Button* mUpdateDisplay;

    // Used with Activity
    QWidget* mActivityOptsGroup;
    Label* mActivityOptsTitle;
    QLabel* mRangeThreshLab;
    LineEdit* mRangeThresholdEdit;
    QLabel* mHActivityLab;
    LineEdit* mHActivityEdit;

    // ------------------------------------
    //  Pagination / Exoprt Tools
    // ------------------------------------
    QWidget* mPageSaveGroup;

    Tabs* mPageSaveTab;
    QWidget* mPageWidget;

    QWidget* mSaveAllWidget;
    QWidget* mSaveSelectWidget;


    Button* mNextPageBut;
    Button* mPreviousPageBut;
    LineEdit* mPageEdit;
    QLabel* mGraphsPerPageLab;
    QSpinBox* mGraphsPerPageSpin;

    Button* mExportImgBut;
    Button* mExportResults;

    Button* mImageSaveBut;
    Button* mImageClipBut;
    Button* mResultsClipBut;
    Button* mDataSaveBut;

    // ----------------------------------------
    //  Useful Variables
    // ----------------------------------------
    GraphViewResults::graph_t mCurrentTypeGraph;
    QList<GraphViewResults::variable_t> mCurrentVariableList;
    GraphViewResults::variable_t mMainVariable;
    bool mHasPhases;
    double mHpdThreshold; // [0 : 100]%
    // ----------------------------------------
    //  Time Span Variables
    // ----------------------------------------
    double mResultZoomT;
    double mResultMinT;
    double mResultMaxT;
    double mResultCurrentMinT;
    double mResultCurrentMaxT;

    // ----------------------------------------
    //  Time Scale ticks intervals
    // ----------------------------------------
    double mMajorScale;
    int mMinorCountScale;

    // ----------------------------------------
    //  X Span  Variables / First curve
    // ----------------------------------------
    double mResultCurrentMinX;
    double mResultCurrentMaxX;

    // ----------------------------------------
    //  Y Span  Variables / Second Curve
    // ----------------------------------------
    double mResultCurrentMinY;
    double mResultCurrentMaxY;

    // ----------------------------------------
    //  Z Span  Variables / Third curve
    // ----------------------------------------
    double mResultCurrentMinZ;
    double mResultCurrentMaxZ;

    // ----------------------------------------
    //  Pagination variables
    // ----------------------------------------
    int mCurrentPage;
    int mGraphsPerPage;
    int mMaximunNumberOfVisibleGraph;

    // ------------------------------------
    QMap<QPair<GraphViewResults::variable_t, GraphViewResults::graph_t>, int> mZoomsH; // Zoom in Graphic Option

    QMap<QPair<GraphViewResults::variable_t, GraphViewResults::graph_t>, QPair<double, double>> mZoomsT;
    QMap<QPair<GraphViewResults::variable_t, GraphViewResults::graph_t>, QPair<double, int>> mScalesT;

    QMap<QPair<GraphViewResults::variable_t, GraphViewResults::graph_t>, QPair<double, double>> mZoomsX;
    QMap<QPair<GraphViewResults::variable_t, GraphViewResults::graph_t>, QPair<double, double>> mZoomsY;
    QMap<QPair<GraphViewResults::variable_t, GraphViewResults::graph_t>, QPair<double, double>> mZoomsZ;
};

#endif
