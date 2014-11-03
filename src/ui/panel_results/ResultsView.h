#ifndef ResultsWrapper_H
#define ResultsWrapper_H

#include "Model.h"
#include <QWidget>

class QScrollArea;
class QVBoxLayout;
class GraphView;
class LineEdit;
class QStackedWidget;

class Tabs;
class ResultsControls;
class ZoomControls;
class Ruler;
class GraphView;
class GraphViewPhase;
class GraphViewEvent;
class GraphViewDate;
class ResultsScroller;
class ResultsMarker;

class Label;
class Button;
class CheckBox;
class RadioButton;


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
    void updateResults(const Model& model);
    void updateOptions();
    void toggleInfos();
    void updateGraphs();
    
private slots:
    void showPhasesScene(bool show);
    void showEventsScene(bool show);
    void changeTab(int index);
    
private:
    int mMargin;
    int mOptionsW;
    int mLineH;
    int mGraphLeft;
    int mRulerH;
    int mTabsH;
    
    Tabs* mTabs;
    Ruler* mRuler;
    QStackedWidget* mStack;
    ResultsScroller* mResultsScrollerPhases;
    ResultsScroller* mResultsScrollerEvents;
    ResultsMarker* mMarker;
    
    Button* mPhasesSceneBut;
    Button* mEventsSceneBut;
    bool mHasPhases;
    bool mShowPhasesScene;
    
    QWidget* mOptionsWidget;
    
    QWidget* mZoomWidget;
    Button* mZoomInBut;
    Button* mZoomDefaultBut;
    Button* mZoomOutBut;
    
    CheckBox* mHPDCheck;
    LineEdit* mHPDEdit;
    
    Label* mChainsTitle;
    Label* mPhasesTitle;
    Label* mDataTitle;
    
    QWidget* mChainsGroup;
    CheckBox* mAllChainsCheck;
    QList<CheckBox*> mCheckChainChecks;
    
    QWidget* mPhasesGroup;
    CheckBox* mAlphaCheck;
    CheckBox* mBetaCheck;
    CheckBox* mPredictCheck;
    
    QWidget* mDataGroup;
    RadioButton* mDataThetaRadio;
    CheckBox* mDataCalibCheck;
    RadioButton* mDataSigmaRadio;
    RadioButton* mDataDeltaRadio;
};

#endif
