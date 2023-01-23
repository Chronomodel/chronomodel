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

#ifndef RESULTSWRAPPER_H
#define RESULTSWRAPPER_H

#include "MCMCLoopChrono.h"
#include "AxisTool.h"
#include "GraphViewResults.h"
#include "AppSettings.h"

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
    ResultsView(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Widget);
    ~ResultsView();

    void setProject(Project* project);
    void initModel(Model* model);
    void updateModel(Model* model);

protected:
    // ------------------------------------------------
    //  Events & Layout
    // ------------------------------------------------
    void mouseMoveEvent(QMouseEvent* e);
    void resizeEvent(QResizeEvent* e);

    void updateMarkerGeometry(const int x);
    void updateGraphsLayout();
    void updateGraphsLayout(QScrollArea* scrollArea, QList<GraphViewResults*> graphs);

    // ------------------------------------------------
    //  Graphs UI
    // ------------------------------------------------
    void createGraphs();
    void createByEventsGraphs();
    void createByPhasesGraphs();
    //void createByTempoGraphs();
    void createByCurveGraph();
    void createByAlphaGraph();
    
    void deleteAllGraphsInList(QList<GraphViewResults*>& list);
    QList<GraphViewResults*> allGraphs();
    QList<GraphViewResults*> currentGraphs(bool onlySelected);
    bool hasSelectedGraphs();
    
    void updateGraphsMinMax();
    double getGraphsMax(const QList<GraphViewResults*>& graphs, const QString& title, double maxFloor);
    double getGraphsMin(const QList<GraphViewResults*>& graphs, const QString& title, double minFloor);

    
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
    void setTimeSpin(const double value);
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
    ModelCurve* modelCurve() const;
    inline bool isCurve() const;

public slots:

    void clearResults(); // connected to Project::mcmcStarted

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
    void generateCurves();
    void updateCurvesToShow();
    void updateScales();
    void updateOptionsWidget();
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
  //  void applyDisplayTab();
   // void applyPageSavingTab();
    
    void applyCurrentVariable();
   // void applyUnfoldEvents();
   // void applyUnfoldDates();

    // Span options
    void applyStudyPeriod();
    void applyTimeRange();
    void applyTimeSlider(int value);
    void applyTimeSpin(double value);
    void applyZoomScale();

    // Graphic options
    void applyZoomSlider(int value);
    void applyZoomSpin(int value);
    void applyFont();
    void applyThickness(const int value);
    void applyOpacity(const int value);

    // Density options
    void applyFFTLength();
    void applyBandwidth();
    void applyThreshold();
    void applyHActivity();


    // ------------------------------------
    //  Page / Save
    // ------------------------------------
    void togglePageSave();

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

    void resultsLogUpdated(const QString &log);

    void xSpinUpdate(const int value);
    void xSlideUpdate(const int value);

public:
    // mModel gives access to :
    // - mModel->mSettings (ProjectSettings)
    // - mModel->mMCMCSettings (MCMCSettings)
    // - mModel->mChains (QList<ChainSpecs>)
    Model* mModel;

private:

    // ---------------------------------------------------------------------
    // UI useful values
    // ---------------------------------------------------------------------
    int mMargin;
    int mOptionsW;
    qreal mMarginLeft;
    qreal mMarginRight;
    int mGraphHeight;
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
    QScrollArea* mCurveScrollArea;

    QList<GraphViewResults*> mByEventsGraphs;
    QList<GraphViewResults*> mByPhasesGraphs;
    QList<GraphViewResults*> mByCurveGraphs;

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

    RadioButton* mEventVGRadio;

    CheckBox* mStatCheck;

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
    RadioButton* mS02VgRadio;

    CheckBox* mCurveErrorCheck;
    CheckBox* mCurveMapCheck;
    CheckBox* mCurveEventsPointsCheck;
    CheckBox* mCurveDataPointsCheck;
    CheckBox* mCurveStatCheck;

    // ---------------------------------------------------------------------
    // Tabs : Display / Distrib. Options
    // ---------------------------------------------------------------------
    Tabs* mDisplayDistribTab;
    QWidget* mDisplayWidget;
    QWidget* mDistribWidget;

    // ---------------------------------------------------------------------
    //  Span options : UI components to manipulate X axis scale
    // ---------------------------------------------------------------------
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
    QLabel* mXLab;
    QSlider* mXSlider;
    QDoubleSpinBox* mTimeSpin;

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

    // Adjust the zoom on the study period
    Button* mXOptionBut;

    // Force the min T and max T
    QLabel* mXOptionLab;
    LineEdit* mCurrentXMinEdit;
    LineEdit* mCurrentXMaxEdit;
    // ------------------------------------
    //  Graphic Options
    // ------------------------------------
    QWidget* mGraphicGroup;
    Label* mGraphicTitle;

    QLabel* mZoomLab;
    QSlider* mZoomSlider;
    QSpinBox* mZoomSpin;

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

    QLabel* mRangeThreshLab;
    LineEdit* mRangeThresholdEdit; // Used with Activity
    Label* mFFTLenLab;
    QComboBox* mFFTLenCombo;
    Label* mBandwidthLab;
    QDoubleSpinBox* mBandwidthSpin;
    Button* mUpdateDisplay;

    Label* mHActivityLab;
    LineEdit* mHActivityEdit;

    // ------------------------------------
    //  Pagination / Exoprt Tools
    // ------------------------------------
    QWidget* mPageSavegWidget;

    Tabs* mPageSaveTab;
    QWidget* mPageWidget;

    QWidget* mSaveAllWidget;
    QWidget* mSaveSelectWidget;


    Button* mNextPageBut;
    Button* mPreviousPageBut;
    QLineEdit* mPageEdit;
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
    QVector<GraphViewResults::variable_t> mCurrentVariableList;
    GraphViewResults::variable_t mMainVariable;
    bool mHasPhases;

    // ----------------------------------------
    //  Time Span / Zoom Variables
    // ----------------------------------------
    double mResultZoomT;
    double mResultMinT;
    double mResultMaxT;
    double mResultCurrentMinT;
    double mResultCurrentMaxT;
    // ----------------------------------------
    //  X Span / Zoom Variables
    // ----------------------------------------
    //double mResultZoomX;
    //double mResultMinX;
    //double mResultMaxX;
    double mResultCurrentMinX;
    double mResultCurrentMaxX;

    // ----------------------------------------
    //  X Scale ticks intervals
    // ----------------------------------------
    double mMajorScale;
    int mMinorCountScale;

    // ----------------------------------------
    //  Pagination variables
    // ----------------------------------------
    int mCurrentPage;
    int mGraphsPerPage;
    int mMaximunNumberOfVisibleGraph;

    // ------------------------------------
    QMap<QPair<GraphViewResults::variable_t, GraphViewResults::graph_t>, QPair<double, double>> mZooms;
    QMap<QPair<GraphViewResults::variable_t, GraphViewResults::graph_t>, QPair<double, int>> mScales;
};

#endif
