#ifndef GraphViewResults_H
#define GraphViewResults_H

#include "ProjectSettings.h"
#include "MCMCSettings.h"
#include "MCMCLoop.h"
#include "GraphView.h"

#include <QWidget>
#include <QList>
#include <QColor>

class Button;
class QPropertyAnimation;
class QTextEdit;


class GraphViewResults: public QWidget
{
    Q_OBJECT
public:
    enum TypeGraph{
        eHisto = 0,
        eTrace = 1,
        eAccept = 2,
        eCorrel = 3
    };
    enum Variable{
        eTheta = 0,
        eSigma = 1
    };
    
    
    explicit GraphViewResults(QWidget *parent = 0);
    virtual ~GraphViewResults();
    
    void setResultToShow(TypeGraph typeGraph, Variable variable, bool showAllChains, const QList<bool>& showChainList, bool showCredibility, float threshold, bool showCalib, bool showWiggle, bool showRawResults);
    
    void setSettings(const ProjectSettings& settings);
    void setMCMCSettings(const MCMCSettings& mcmc, const QList<Chain>& chains);
    
    void setMainColor(const QColor& color);
    void toggle(const QRect& geometry);
    
    void setRendering(GraphView::Rendering render);
    virtual void setGraphFont(const QFont& font);
    void setGraphsThickness(int value);
    
    void setItemColor(const QColor& itemColor);
    void setItemTitle(const QString& itemTitle);
    
    void forceHideButtons(const bool hide);
    
    GraphView* mGraph;
    
public slots:
    void setRange(double min, double max);
    void zoom(double min, double max);
    void showNumericalResults(bool show);
    void setNumericalResults(const QString& results);
    
private slots:
    void saveAsImage();
    void imageToClipboard();
    void resultsToClipboard();
    void saveGraphData() const;
    
protected:
    // These methods are from QWidget and we want to modify their behavior
    virtual void paintEvent(QPaintEvent* e);
    virtual void resizeEvent(QResizeEvent* e);
    
    // This is not from QWidget : we create this function to update the layout from different places (eg: in resizeEvent()).
    // It is vitual beacause we want a different behavior in suclasses (GraphViewDate, GraphViewEvent and GraphViewPhase)
    virtual void updateLayout();
    
    // This method is used to recreate all curves in mGraph.
    // It is vitual beacause we want a different behavior in suclasses (GraphViewDate, GraphViewEvent and GraphViewPhase)
    virtual void refresh() = 0;
    
signals:
    void unfoldToggled(bool toggled);
    void visibilityChanged(bool visible);
    
protected:
    
    
    TypeGraph mCurrentTypeGraph;
    Variable mCurrentVariable;
    
    QString mTitle;
    
    QString mResults;
    QColor mItemColor;
    QString mItemTitle;
    
    bool mShowAllChains;
    QList<bool> mShowChainList;
    bool mShowCredibility;
    //int mThresholdHPD;
    float mThresholdHPD;
    
    bool mShowCalib;
    bool mShowWiggle;
    bool mShowRawResults;
    bool mShowNumResults;
    
    ProjectSettings mSettings;
    MCMCSettings mMCMCSettings;
    QList<Chain> mChains;
    
    QColor mMainColor;
    
    QTextEdit* mTextArea;
    
    Button* mImageSaveBut;
    Button* mImageClipBut;
    Button* mResultsClipBut;
    Button* mDataSaveBut;
    
    int mMargin;
    int mLineH;
    int mGraphLeft;
    int mTopShift;
    
    bool mButtonVisible;
    bool mForceHideButtons;
    int mMinHeightForButtonsVisible;
    
    QPropertyAnimation* mAnimation;
};

#endif
