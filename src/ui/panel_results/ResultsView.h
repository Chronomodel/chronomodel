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
    
    void doProjectConnections(Project* project);
    
protected:
    void paintEvent(QPaintEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
    void resizeEvent(QResizeEvent* e);
    void updateLayout();
    
public slots:
    void clearResults();
    void updateResults(Model* model);
    void updateGraphs();
    void updateRulerAreas();
    void updateModel();
    
private slots:
    void updateScaleY(int value);
    void updateScaleX(int value);
    void updateScroll(double min, double max);
    
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
    
private:
    Model* mModel;
    ProjectSettings mSettings;
    MCMCSettings mMCMCSettings;
    QList<Chain> mChains;
    
    double mMinX;
    double mMaxX;
    
    int mMargin;
    int mOptionsW;
    int mLineH;
    int mGraphLeft;
    int mRulerH;
    int mTabsH;
    int mGraphsH;
    
    Tabs* mTabs;
    Ruler* mRuler;
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
    
    /*QWidget* mZoomWidget;
    Button* mZoomInBut;
    Button* mZoomDefaultBut;
    Button* mZoomOutBut;*/
    
    Label* mDisplayTitle;
    QWidget* mDisplayGroup;
    Button* mUnfoldBut;
    Button* mInfosBut;
    Button* mExportImgBut;
    
    Label* mXScaleLab;
    Label* mYScaleLab;
    QSlider* mXSlider;
    QSlider* mYSlider;
    Label* mRenderLab;
    QComboBox* mRenderCombo;
    
    Label* mChainsTitle;
    QWidget* mChainsGroup;
    CheckBox* mAllChainsCheck;
    QList<CheckBox*> mCheckChainChecks;
    QList<RadioButton*> mChainRadios;
    
    Label* mDataTitle;
    QWidget* mDataGroup;
    RadioButton* mDataThetaRadio;
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
    
    int mZoomDensity;
    int mZoomTrace;
    int mZoomAccept;
    int mZoomCorrel;
};

#endif
