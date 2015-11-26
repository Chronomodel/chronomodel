#ifndef ResultsWrapper_H
#define ResultsWrapper_H

#include <QWidget>
#include "MCMCLoopMain.h"
#include "AxisTool.h"
#include "GraphViewResults.h"

class QStackedWidget;
class QScrollArea;
class QTimer;
class QComboBox;
class QSlider;
class QScrollBar;
class QSpinBox;
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
    ResultsView(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~ResultsView();
    
    double mResultZoomX;
    double mResultCurrentMinX;
    double mResultCurrentMaxX;
    double mResultMinX;
    double mResultMaxX;
    bool mHasPhases;
    
    void doProjectConnections(Project* project);
    
protected:
    void paintEvent(QPaintEvent* );
    void mouseMoveEvent(QMouseEvent* e);
    void resizeEvent(QResizeEvent* e);
    
    void createEventsScrollArea();
    void createPhasesScrollArea();

public slots:
    void updateLayout();
    void updateGraphsLayout();
    
    void clearResults();
    void updateResults(Model* model = 0);
    
    void generatePosteriorDistribs();
    void generateCredibilityAndHPD();
    void generateCurves();
    void updateCurvesToShow();
    
    void updateControls();
    void updateScales();
    
    void updateModel();
    void updateResultsLog();
private slots:

    void settingChange();
    void updateZoomX(); // Connected to slider signals
    void updateScroll(const double min, const double max); // Connected to ruler signals
    void editCurrentMinX(); // Connected to min edit signals
    void editCurrentMaxX(); // Connected to max edit signals
    void updateZoomEdit();
    void updateGraphsZoomX();
    
    void updateScaleY(int value);
    
    void updateFont();
    void updateThickness(int value);
    void updateOpacity(int value);
    void updateRendering(int index);
    void showInfos(bool);
    void exportFullImage();
    void exportResults();
    
signals:
    void posteriorDistribGenerated();
    void credibilityAndHPDGenerated();
    void curvesGenerated();
    
    void controlsUpdated();
    void resultsLogUpdated(const QString& log);
    
private:
    QList<QRect> getGeometries(const QList<GraphViewResults*>& graphs, bool open, bool byPhases);
    

    void clearHisto();
    void clearChainHistos();
    
private:
    GraphViewResults::TypeGraph mCurrentTypeGraph;
    Ruler* mRuler;
    
    Model* mModel;
    ProjectSettings mSettings;
    MCMCSettings mMCMCSettings;
    QList<Chain> mChains;    
    
    int mMargin;
    int mOptionsW;
    int mLineH;
    int mGraphLeft;
    int mRulerH;
    int mTabsH;
    int mGraphsH;
    
    Tabs* mTabs;
    //Ruler* mRuler;
    Marker* mMarker;
    
    QStackedWidget* mStack;
    QScrollArea* mEventsScrollArea;
    QScrollArea* mPhasesScrollArea;
    QList<GraphViewResults*> mByEventsGraphs;
    QList<GraphViewResults*> mByPhasesGraphs;
    
    Button* mByPhasesBut;
    Button* mByEventsBut;
    
    
    QWidget* mOptionsWidget;
    
    
    Button* mUnfoldBut;
    Button* mInfosBut;
    Button* mExportImgBut;
    Button* mExportResults;
    CheckBox* mShowDataUnderPhasesCheck;
    
    // ------ mDisplayGroup -----
   // QWidget* mScaleGroup;
    Label* mDisplayTitle;
    QWidget* mDisplayGroup;
    Label* mXScaleLab;
    Label* mYScaleLab;
    QSlider* mXSlider;
    QSlider* mYSlider;
    LineEdit* mCurrentXMinEdit;
    LineEdit* mCurrentXMaxEdit;
    
    QFont mFont;
    QPushButton* mFontBut;
    QSpinBox* mThicknessSpin;
    QSpinBox* mOpacitySpin;
    QComboBox* mRenderCombo;
    
    
    Label* mChainsTitle;
    QWidget* mChainsGroup;
    CheckBox* mAllChainsCheck;
    QList<CheckBox*> mCheckChainChecks;
    QList<RadioButton*> mChainRadios;
    
    Label* mResultsTitle;
    QWidget* mResultsGroup;
    RadioButton* mDataThetaRadio;
    
    CheckBox* mDataCalibCheck;
    CheckBox* mWiggleCheck;
    RadioButton* mDataSigmaRadio;
    
    Label* mPostDistOptsTitle;
    QWidget* mPostDistGroup;
    Label* mThreshLab;
    CheckBox* mCredibilityCheck;
    LineEdit* mHPDEdit;
    Label* mFFTLenLab;
    QComboBox* mFFTLenCombo;
    Label* mHFactorLab;
    LineEdit* mHFactorEdit;
    Button* mUpdateDisplay;
    
    int mComboH;
    
    QMap<int, QPair<double, double>> mZooms;
};

#endif
