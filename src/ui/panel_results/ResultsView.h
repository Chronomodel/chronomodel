#ifndef ResultsWrapper_H
#define ResultsWrapper_H

#include "Model.h"
#include <QWidget>

class QScrollArea;
class QVBoxLayout;
class GraphView;
class LineEdit;
class QStackedWidget;

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
class GroupBox;


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
    
private:
    int mMargin;
    int mOptionsW;
    int mLineH;
    int mGraphLeft;
    int mRulerH;
    
    Ruler* mRuler;
    QStackedWidget* mStack;
    ResultsScroller* mResultsScrollerPhases;
    ResultsScroller* mResultsScrollerEvents;
    ResultsMarker* mMarker;
    
    Button* mPhasesSceneBut;
    Button* mEventsSceneBut;
    bool mHasPhases;
    bool mShowPhasesScene;
    
    QRectF mOptionsRect;
    
    QRectF mZoomRect;
    Button* mZoomInBut;
    Button* mZoomDefaultBut;
    Button* mZoomOutBut;
    
    GroupBox* mTypeGroup;
    RadioButton* mHistoRadio;
    CheckBox* mHPDCheck;
    LineEdit* mHPDEdit;
    RadioButton* mTraceRadio;
    RadioButton* mAcceptRadio;
    
    GroupBox* mChainsGroup;
    CheckBox* mAllChainsCheck;
    QList<CheckBox*> mCheckChainChecks;
    
    GroupBox* mPhasesGroup;
    CheckBox* mAlphaCheck;
    CheckBox* mBetaCheck;
    CheckBox* mPredictCheck;
    
    GroupBox* mDataGroup;
    RadioButton* mDataThetaRadio;
    CheckBox* mDataCalibCheck;
    RadioButton* mDataSigmaRadio;
    RadioButton* mDataDeltaRadio;
};

#endif
