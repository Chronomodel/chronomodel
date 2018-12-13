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

    double mResultZoomX;
    double mResultCurrentMinX;
    double mResultCurrentMaxX;

    // avalable for Event
    double mResultMinX;
    double mResultMaxX;

    // avalable for Variance
    double mResultMaxVariance;

    double mResultMaxDuration;

    bool mHasPhases;

    Model* mModel;

    void doProjectConnections(Project* project);

    void updateFormatSetting(Model* model);
    double getBandwidth() const;
    int getFFTLength() const;
    double getThreshold() const;


protected:
    void setGraphFont(const QFont &font);

    void mouseMoveEvent(QMouseEvent* e);
    void resizeEvent(QResizeEvent* e);

    void createEventsScrollArea(const int idx = 0);
    void createPhasesScrollArea(const int idx = 0);
    void createTempoScrollArea(const int idx = 0);
    void generateCurves(const QList<GraphViewResults*>& listGraphs);

    void updateTabDisplay(const int &i);
    void updateTabByScene();
    void updateTabByTempo();
    void updateTabPageSaving();
    void updateNbDensity(int i);

public slots:
    void updateResults(Model* model = nullptr);
    void initResults(Model* model = nullptr);

    void changeScrollArea();
    void updateLayout();
    void updateGraphsLayout();

    void clearResults();
    void updateCurves();


    void updateControls();
    void applyAppSettings();
    void updateScales();

    void updateModel();
    void updateResultsLog();

private slots:
    void updateVisibleTabs(const int &index);
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

    void updateScaleY(int value);
    // connected to mMajorScaleEdit and mMinorScaleEdit
    void updateScaleX();
    void updateScaleEdit();


    void updateGraphFont();
    void updateThickness(const int value);
    void updateOpacity(const int value);
   // void updateRendering(int index);
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
   // int mLineH;
    //used for graph
    qreal mMarginLeft;
    qreal mMarginRight;
    int mRulerH;
    int mTabsH;
    int mGraphHeight;

    Tabs* mTabs;
    int mTabEventsIndex;
    int mTabPhasesIndex;
    int mTabTempoIndex;

    Marker* mMarker;

    QStackedWidget* mStack;
    QScrollArea* mEventsScrollArea;
    QScrollArea* mPhasesScrollArea;
    QScrollArea* mTempoScrollArea;

    QList<GraphViewResults*> mByEventsGraphs;
    QList<GraphViewResults*> mByPhasesGraphs;
    QList<GraphViewResults*> mByTempoGraphs;

    QWidget* mOptionsWidget;


    // --- Variables
    Tabs* mTabByScene; // replace mByEventsBut and mByPhasesBut
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

    Label* mMajorScaleLab;
    LineEdit* mMajorScaleEdit;
    Label* mMinorScaleLab;
    LineEdit* mMinorScaleEdit;
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

    QFont mGraphFont;
    Button* mFontBut;
    QComboBox* mThicknessCombo;
    QComboBox* mOpacityCombo;

    QLabel* mLabFont;
    QLabel * mLabThickness;
    QLabel * mLabOpacity;

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


    Tabs* mTabPageSaving;
    QWidget* mPageWidget;
    // Page Navigator
    Button* mNextSheetBut;
    LineEdit* mSheetNum;
    Button* mPreviousSheetBut;

    Label* mNbDensityLab;
    QSpinBox* mNbDensitySpin;

    QWidget* mToolsWidget;
    Button* mExportImgBut;
    Button* mExportResults;

    Button* mImageSaveBut;
    Button* mImageClipBut;
    Button* mResultsClipBut;
    Button* mDataSaveBut;


    int mComboH;

    QMap<QPair<GraphViewResults::Variable, GraphViewResults::TypeGraph>, QPair<double, double>> mZooms;
    QMap<QPair<GraphViewResults::Variable, GraphViewResults::TypeGraph>, QPair<double, int>> mScales;
    //propreties
    GraphViewResults::TypeGraph mCurrentTypeGraph;
    GraphViewResults::Variable mCurrentVariable;
    double mBandwidthUsed;
    double mThresholdUsed;
    int mNumberOfGraph;
    int mMaximunNumberOfVisibleGraph;
    double mMajorScale;
    int mMinorCountScale;


    int titleHeight ;
    int labelHeight ;
    int lineEditHeight;
    int checkBoxHeight;
    int comboBoxHeight ;
    int radioButtonHeight;
    int spinBoxHeight;
    int buttonHeight;

};

#endif
