#ifndef ResultsWrapper_H
#define ResultsWrapper_H

#include <QWidget>
#include <QVBoxLayout>
#include <QTabWidget>
#include "MCMCLoopMain.h"
#include "AxisTool.h"
#include "GraphViewResults.h"
#include "AppSettings.h"

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
    ResultsView(QWidget* parent = nullptr, Qt::WindowFlags flags = 0);
    ~ResultsView();
    
    double mResultZoomX;
    double mResultCurrentMinX;
    double mResultCurrentMaxX;

    // avalable for Event
    double mResultMinX;
    double mResultMaxX;

    // avalable for Date
    //double mResultMinDateX;
    //double mResultMaxDateX;

    // avalable for Variance
    double mResultMaxVariance;

    double mResultMaxDuration;

    bool mHasPhases;
    
    Model* mModel;

    void doProjectConnections(Project* project);

    void updateFormatSetting(Model* model, const AppSettings* appSet);
    double getBandwidth() const;
    int getFFTLength() const;
    double getThreshold() const;

    void setFont(const QFont & font);

protected:
    void paintEvent(QPaintEvent* );
    void mouseMoveEvent(QMouseEvent* e);
    void resizeEvent(QResizeEvent* e);

    void createEventsScrollArea(const int idx = 0);
    void createPhasesScrollArea(const int idx = 0);
    void generateCurves(const QList<GraphViewResults*>& listGraphs);

    void updateTabDisplay(const int &i);
    void updateTabByScene();
    void updateTabPageTools();

public slots:
    void updateResults(Model* model = nullptr);
    void initResults(Model* model = nullptr);

    void changeScrollArea();
    void updateLayout();
    void updateGraphsLayout();
    
    void clearResults();
    void updateCurves();
    
    
    void updateControls();
    void updateScales();
    
    void updateModel();
    void updateResultsLog();

  //  void adjustDuration(bool visible);

private slots:
    void graphTypeChange();
    void updateCurvesToShow();
    
    void settingChange();
    void updateZoomX(); // Connected to slider signals
    void updateScroll(const double min, const double max); // Connected to ruler signals
    void editCurrentMinX(); // Connected to min edit signals
    void editCurrentMaxX(); // Connected to max edit signals
    void setStudyPeriod(); // connected to study button
    void updateZoomEdit();
    void updateGraphsZoomX();
    
    void setXScaleSpin(const double value); // connected to mXScaleSpin
    void XScaleSpinChanged(double value);
    void setXScaleSlide(const int value);
    void XScaleSliderChanged( int value);
    void updateScaleX();
    void updateScaleY(int value);
    
    void updateFont();
    void updateThickness(const int value);
    void updateOpacity(const int value);
    void updateRendering(int index);
    void showInfos(bool);
    void exportFullImage();
    void exportResults();

    void saveAsImage();
    void imageToClipboard();
    void resultsToClipboard();
    void saveGraphData();

    void previousSheet();
    void nextSheet();
    void unfoldToggle();

 //   void showTabDisplay( const int &i);

    // SETTER
    void setFFTLength();
    void setBandwidth();
    void setThreshold();

signals:
   
    void curvesGenerated();
    
    void controlsUpdated();
    void resultsLogUpdated(const QString &log);
    
    void scalesUpdated();
    
    void updateScrollAreaRequested();
    void generateCurvesRequested();

    void xSpinUpdate(const int value);
    void xSlideUpdate(const int value);

    
private:
    void clearHisto();
    void clearChainHistos();
    double sliderToZoom(const int &coef);
    int zoomToSlider(const double &zoom);
    Ruler* mRuler;

    ProjectSettings mSettings;
    MCMCSettings mMCMCSettings;
    QList<ChainSpecs> mChains;
    
    // used for options side
    int mMargin;
    int mOptionsW;
    int mLineH;
    //used for graph
    int mGraphLeft;
    int mRulerH;
    int mTabsH;
    int mGraphsH;
    
    Tabs* mTabs;
    int mTabEventsIndex;
    int mTabPhasesIndex;

    Marker* mMarker;

    QStackedWidget* mStack;
    QScrollArea* mEventsScrollArea;
    QScrollArea* mPhasesScrollArea;

    QList<GraphViewResults*> mByEventsGraphs;
    QList<GraphViewResults*> mByPhasesGraphs;

    QWidget* mOptionsWidget;



    Tabs* mTabPageTools;
    QWidget* mPageWidget;
    // Page Navigator
    Label* mSheetTitle;
    Button* mNextSheetBut;
    Button* mPreviousSheetBut;

    QWidget* mToolsWidget;
    Button* mStatsBut;
    Button* mExportImgBut;
    Button* mExportResults;

    Button* mImageSaveBut;
    Button* mImageClipBut;
    Button* mResultsClipBut;
    Button* mDataSaveBut;



    // --- Variables
    Tabs* mTabByScene; // replace mByEventsBut and mByPhasesBut
    QWidget* mResultsGroup;

    CheckBox* mEventsfoldCheck;
    CheckBox* mDatesfoldCheck;
    RadioButton* mDataThetaRadio;

    CheckBox* mDataCalibCheck;
    CheckBox* mWiggleCheck;
    RadioButton* mDataSigmaRadio;
    RadioButton* mPhaseDurationRadio;



    // -- tabs

    Tabs *mTabDisplayMCMC;
    QWidget* mTabDisplay;
    QWidget* mTabMCMC;

    // ------ Span Options -----
    QWidget* mSpanGroup;
    Label* mSpanTitle;

    Button* mDisplayStudyBut;
    Label* mSpanLab;
    LineEdit* mCurrentXMinEdit;
    LineEdit* mCurrentXMaxEdit;

    Label* mXScaleLab;
    QSlider* mXSlider;
    QDoubleSpinBox* mXScaleSpin;
    /* used to controle the signal XScaleSpin::valueChanged () when we need to change the value
     * xScaleChanged(int value)
     * emit xScaleUpdate(value);
     */
    bool forceXSpinSetValue;
    bool forceXSlideSetValue;

    // ------ Graphic options - (old mDisplayGroup) -----
    QWidget* mGraphicGroup;
    Label* mGraphicTitle;

    Label* mYScaleLab;

    QSlider* mYSlider;
    QSpinBox* mYScaleSpin;

    QFont mFont;
    Button* mFontBut;
    QSpinBox* mThicknessSpin;
    QSpinBox* mOpacitySpin;
    QComboBox* mRenderCombo;
    
    Label* labFont;
    Label* labThickness;
    Label* labOpacity;
    Label* labRendering;
    //------------ MCMC Chains---------
    QWidget* mChainsGroup;
    Label* mChainsTitle;

    CheckBox* mAllChainsCheck;
    QList<CheckBox*> mCheckChainChecks;
    QList<RadioButton*> mChainRadios;
    

    
    //--------- Density Options
    QWidget* mDensityOptsGroup;
    Label* mDensityOptsTitle;

    Label* mThreshLab;
    CheckBox* mCredibilityCheck;
    LineEdit* mHPDEdit;
    Label* mFFTLenLab;
    QComboBox* mFFTLenCombo;
    Label* mBandwidthLab;
    LineEdit* mBandwidthEdit;
    Button* mUpdateDisplay;

    int mComboH;
    
   // QMap<int, QPair<double, double>> mZooms;
    QMap<QPair<GraphViewResults::Variable, GraphViewResults::TypeGraph>, QPair<double, double>> mZooms;
    //propreties
    GraphViewResults::TypeGraph mCurrentTypeGraph;
    GraphViewResults::Variable mCurrentVariable;
    double mBandwidthUsed;
    double mThresholdUsed;
    int mNumberOfGraph;
    int mMaximunNumberOfVisibleGraph;
};

#endif
