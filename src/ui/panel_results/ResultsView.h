#ifndef ResultsWrapper_H
#define ResultsWrapper_H

#include <QWidget>
#include "MCMCLoopMain.h"

class QStackedWidget;
class QScrollArea;
class QTimer;
class QComboBox;
class QSlider;
class QScrollBar;
class QSpinBox;

class Project;
class Model;
class Tabs;
class Ruler;
class GraphView;
class GraphViewResults;
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
    
    enum TypeGraph{
        eHisto = 0,
        eTrace = 1,
        eAccept = 2,
        eCorrel = 3
    };
    
    double mResultZoomX;
    double mResultCurrentMinX;
    double mResultCurrentMaxX;
    double mResultMinX;
    double mResultMaxX;
    void doProjectConnections(Project* project);
    void updateAllZoom();
    TypeGraph mCurrentTypeGraph;
    
    Ruler* mRuler;
    
protected:
    void paintEvent(QPaintEvent* );
    void mouseMoveEvent(QMouseEvent* e);
    void resizeEvent(QResizeEvent* e);
    void updateLayout();
    
public slots:
    void clearResults();
    void updateResults(Model* model = 0);
    void updateGraphs();
    void updateRulerAreas();
    void updateModel();
    
private slots:
    void updateScaleY(int value);
    void updateZoomX(int value);
    void withSlider();
    
    void updateScroll(const double min, const double max);
    void updateRuler(int value);
    
    void setCurrentMinX();
    void editCurrentMinX(QString str);
    void setCurrentMaxX();
    void editCurrentMaxX(QString str);
    
    void updateRendering(int index);
    void showByPhases(bool show);
    void showByEvents(bool show);
    void changeTab(int index);
    void unfoldResults(bool);
    void updateScrollHeights();
    void showInfos(bool);
    void exportFullImage();
    void generateHPD();
    void updateFFTLength();
    void updateHFactor();
    
    void updateFont();
    void updateThickness(int value);
    
signals:
    void resultsLogUpdated(const QString& log);
    
private:
    QList<QRect> getGeometries(const QList<GraphViewResults*>& graphs, bool open, bool byPhases);
    
    void updateResultsLog();
    void memoZoom(const double& zoom);
    void restoreZoom();
    void clearHisto();
    void clearRawHisto();
    void clearChainHistos();
    void clearCredibilityAndHPD();
    
private:
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
    bool mHasPhases;
    
    QWidget* mOptionsWidget;
    
    
    Button* mUnfoldBut;
    Button* mInfosBut;
    Button* mExportImgBut;
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
    Label* mRenderLab;
    QComboBox* mRenderCombo;
    
       
    Label* mChainsTitle;
    QWidget* mChainsGroup;
    CheckBox* mAllChainsCheck;
    QList<CheckBox*> mCheckChainChecks;
    QList<RadioButton*> mChainRadios;
    
    Label* mResultsTitle;
    QWidget* mResultsGroup;
    RadioButton* mDataThetaRadio;
    
    //CheckBox* mDataPosteriorCheck; // new PhD //suppr le 28/04/2015
    CheckBox* mDataCalibCheck;
    CheckBox* mWiggleCheck;
    RadioButton* mDataSigmaRadio;
    
    Label* mPostDistOptsTitle;
    QWidget* mPostDistGroup;
    Label* mThreshLab;
    CheckBox* mHPDCheck;
    LineEdit* mHPDEdit;
    CheckBox* mRawCheck;
    Label* mFFTLenLab;
    QComboBox* mFFTLenCombo;
    Label* mHFactorLab;
    LineEdit* mHFactorEdit;
    Button* mUpdateDisplay;
    
    Button* mFontBut;
    QFont mFont;
    
    Label* mThicknessLab;
    QSpinBox* mThicknessSpin;
    
    int mComboH;
    
    QTimer* mTimer;
    
    double mZoomDensity;
    double mZoomTrace;
    double mZoomAccept;
    double mZoomCorrel;
};

#endif
