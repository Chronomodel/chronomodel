#ifndef ResultsWrapper_H
#define ResultsWrapper_H

#include <QWidget>
#include "MCMCLoopMain.h"

class QStackedWidget;
class QScrollArea;
class QTimer;

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
class ScrollCompressor;


class ResultsView: public QWidget
{
    Q_OBJECT
public:
    ResultsView(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~ResultsView();
    
protected:
    void paintEvent(QPaintEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
    void resizeEvent(QResizeEvent* e);
    void updateLayout();
    
public slots:
    void clearResults();
    void updateResults(MCMCLoopMain&);
    void updateGraphs();
    void updateChains();
    void updateHPD();
    void updateRulerAreas();
    
private slots:
    void setGraphZoom(float min, float max);
    void showByPhases(bool show);
    void showByEvents(bool show);
    void changeTab(int index);
    void unfoldResults(bool);
    void updateScrollHeights();
    void showInfos(bool);
    void compress(float prop);
    
private:
    QList<QRect> getGeometries(const QList<GraphViewResults*>& graphs, bool open, bool byPhases);
    
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
    
    QWidget* mZoomWidget;
    Button* mZoomInBut;
    Button* mZoomDefaultBut;
    Button* mZoomOutBut;
    
    CheckBox* mHPDCheck;
    LineEdit* mHPDEdit;
    
    Label* mChainsTitle;
    Label* mDataTitle;
    
    QWidget* mChainsGroup;
    CheckBox* mAllChainsCheck;
    QList<CheckBox*> mCheckChainChecks;
    QList<RadioButton*> mChainRadios;
    
    QWidget* mDataGroup;
    RadioButton* mDataThetaRadio;
    CheckBox* mDataCalibCheck;
    RadioButton* mDataSigmaRadio;
    RadioButton* mDataDeltaRadio;
    
    Label* mDisplayTitle;
    QWidget* mDisplayWidget;
    Button* mUnfoldBut;
    Button* mInfosBut;
    ScrollCompressor* mCompressor;
    
    QTimer* mTimer;
};

#endif
