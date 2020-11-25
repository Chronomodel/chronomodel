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

#ifndef RESULTSWRAPPER_H
#define RESULTSWRAPPER_H

#include "MCMCLoopMain.h"
#include "AxisTool.h"
#include "GraphViewResults.h"
#include "AppSettings.h"

#include <QVBoxLayout>
#include <QTabWidget>
#include <QLabel>

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
class ModelChronocurve;
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
    void createByTempoGraphs();
    void createByCurveGraph();
    
    void deleteAllGraphsInList(QList<GraphViewResults*>& list);
    QList<GraphViewResults*> allGraphs();
    QList<GraphViewResults*> currentGraphs(bool onlySelected);
    bool hasSelectedGraphs();
    
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
    //  Set controls UI values
    // ------------------------------------------------
    void setXRange();
    void setXSlider(const int value);
    void setXSpin(const double value);
    void setXIntervals();
    
    // ------------------------------------------------
    //  Utilities
    // ------------------------------------------------
    bool isPostDistribGraph();
    bool xScaleRepresentsTime();
    double sliderToZoom(const int &coef);
    int zoomToSlider(const double &zoom);
    
    // ------------------------------------------------
    //  Controls actions helpers
    // ------------------------------------------------
    void updateZoomX();
    void updateGraphsZoomX();
    void updateGraphsHeight();
    
    // ------------------------------------------------
    //  Chronocurve
    // ------------------------------------------------
    ModelChronocurve* modelChronocurve() const;
    bool isChronocurve() const;
    
public slots:
    
    // ------------------------------------------------
    //  Results
    // ------------------------------------------------
    void clearResults(); // connected to Project::mcmcStarted
    void updateResultsLog();

private slots:
    
    // ------------------------------------------------
    //  Layout
    // ------------------------------------------------
    void updateLayout();
    
    // ------------------------------------------------
    //  Graphs / Curves / Controls
    // ------------------------------------------------
    void generateCurves();
    void updateCurvesToShow();
    void updateScales();
    void updateControls();
    
    // ------------------------------------
    //  Tabs changed
    // ------------------------------------
    void setGraphTypeTab(int tabIndex);
    void setGraphListTab(int tabIndex);
    void setDisplayTab(int tabIndex);
    void setPageSavingTab(int tabIndex);
    
    // ------------------------------------
    //  Controls actions
    // ------------------------------------
    void applyRuler(const double min, const double max);
    
    void applyCurrentVariable();
    void applyUnfoldEvents();
    void applyUnfoldDates();
    
    void applyStudyPeriod();
    void applyXRange();
    void applyXSlider(int value);
    void applyXSpin(double value);
    void applyXIntervals();
    
    void applyYSlider(int value);
    void applyYSpin(int value);
    void applyFont();
    void applyThickness(const int value);
    void applyOpacity(const int value);
    
    void applyFFTLength();
    void applyBandwidth();
    void applyThreshold();
    
    void applyGraphsPerPage(int i);
    void applyPreviousPage();
    void applyNextPage();
    
    
    void showInfos(bool);
    
    // ------------------------------------
    //  Saving / Export
    // ------------------------------------
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
    
    // ---------------------------------------------------------------------
    // Left part UI components
    // ---------------------------------------------------------------------
    Tabs* mGraphTypeTabs;
    Ruler* mRuler;
    Marker* mMarker;
    
    QScrollArea* mEventsScrollArea;
    QScrollArea* mPhasesScrollArea;
    QScrollArea* mTempoScrollArea;
    QScrollArea* mCurveScrollArea;
    
    QList<GraphViewResults*> mByEventsGraphs;
    QList<GraphViewResults*> mByPhasesGraphs;
    QList<GraphViewResults*> mByTempoGraphs;
    QList<GraphViewResults*> mByCurveGraphs;

    // ---------------------------------------------------------------------
    // Right UI part components
    // ---------------------------------------------------------------------
    QScrollArea* mOptionsScroll;
    QWidget* mOptionsWidget;
    Tabs* mGraphListTab;
    
    QWidget* mResultsGroup;

    CheckBox* mEventsfoldCheck;
    CheckBox* mDatesfoldCheck;
    RadioButton* mDataThetaRadio;

    CheckBox* mDataCalibCheck;
    CheckBox* mWiggleCheck;
    RadioButton* mDataSigmaRadio;
    CheckBox* mStatCheck;

    QWidget* mTempoGroup;
    RadioButton* mDurationRadio;
    RadioButton* mTempoRadio;
    CheckBox* mTempoCredCheck;
    CheckBox* mTempoErrCheck;
    RadioButton* mActivityRadio;
    CheckBox* mTempoStatCheck;

    // ---------------------------------------------------------------------
    // Tabs : Display / Distrib. Options
    // ---------------------------------------------------------------------
    Tabs* mTabDisplayMCMC;
    QWidget* mTabDisplay;
    QWidget* mTabMCMC;

    // ---------------------------------------------------------------------
    //  Span options : UI components to manipulate X axis scale
    // ---------------------------------------------------------------------
    QWidget* mSpanGroup;
    Label* mSpanTitle;

    // Adjust the zoom on the study period
    Button* mDisplayStudyBut;
    
    // Force the min X and max X
    Label* mSpanLab;
    LineEdit* mCurrentXMinEdit;
    LineEdit* mCurrentXMaxEdit;

    // On the X Axis scale : choose to see the whole graph at once,
    // or zoom on it adjusting the "XScale"
    Label* mXLab;
    QSlider* mXSlider;
    QDoubleSpinBox* mXSpin;

    // On the X Axis scale : choose the major interval between 2 displayed values
    Label* mMajorScaleLab;
    LineEdit* mMajorScaleEdit;
    
    // On the X Axis scale : choose the number of subdivisions between 2 displyed values
    Label* mMinorScaleLab;
    LineEdit* mMinorScaleEdit;
    
    // ------------------------------------
    //  Graphic Options
    // ------------------------------------
    QWidget* mGraphicGroup;
    Label* mGraphicTitle;

    Label* mYLab;
    QSlider* mYSlider;
    QSpinBox* mYSpin;
    
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
    Label* mThreshLab;
    CheckBox* mCredibilityCheck;
    LineEdit* mThresholdEdit;
    Label* mFFTLenLab;
    QComboBox* mFFTLenCombo;
    Label* mBandwidthLab;
    LineEdit* mBandwidthEdit;
    Button* mUpdateDisplay;

    // ------------------------------------
    //  Pagination / Exoprt Tools
    // ------------------------------------
    Tabs* mTabPageSaving;
    QWidget* mPageWidget;
    QWidget* mToolsWidget;
    
    Button* mNextPageBut;
    Button* mPreviousPageBut;
    LineEdit* mPageEdit;
    Label* mGraphsPerPageLab;
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
    GraphViewResults::TypeGraph mCurrentTypeGraph;
    GraphViewResults::Variable mCurrentVariable;
    bool mHasPhases;
    
    // ----------------------------------------
    //  X Span / Zoom Variables
    // ----------------------------------------
    double mResultZoomX;
    double mResultMinX;
    double mResultMaxX;
    double mResultCurrentMinX;
    double mResultCurrentMaxX;
    double mResultMaxVariance;
    double mResultMaxDuration;
    
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
    QMap<QPair<GraphViewResults::Variable, GraphViewResults::TypeGraph>, QPair<double, double>> mZooms;
    QMap<QPair<GraphViewResults::Variable, GraphViewResults::TypeGraph>, QPair<double, int>> mScales;
};

#endif
