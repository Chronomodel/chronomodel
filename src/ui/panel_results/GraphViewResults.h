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
    enum Result{
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
    
    void setResultToShow(Result result, Variable variablee, bool showAllChains, const QList<bool>& showChainList, bool showHpd, float threshold, bool showCalib, bool showWiggle, bool showRawResults);
    
    void setSettings(const ProjectSettings& settings);
    void setMCMCSettings(const MCMCSettings& mcmc, const QList<Chain>& chains);
    
    void setMainColor(const QColor& color);
    void toggle(const QRect& geometry);
    
    void setRendering(GraphView::Rendering render);
    virtual void setGraphFont(const QFont& font);
    void setGraphsThickness(int value);
    
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
    virtual void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* e);
    virtual void updateLayout();
    
    virtual void refresh() = 0;
    
signals:
    void unfoldToggled(bool toggled);
    void visibilityChanged(bool visible);
    
protected:
    GraphView* mGraph;
    
    Result mCurrentResult;
    Variable mCurrentVariable;
    
    QString mTitle;
    int mMinHeighttoDisplayTitle;
    
    QString mResults;
    bool mShowAllChains;
    QList<bool> mShowChainList;
    bool mShowHPD;
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
    
    QPropertyAnimation* mAnimation;
};

#endif
