#ifndef GraphViewResults_H
#define GraphViewResults_H

#include "ProjectSettings.h"
#include "MCMCSettings.h"
#include "MCMCLoop.h"

#include <QWidget>
#include <QList>
#include <QColor>

class GraphView;
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
        eSigma = 1,
        eDelta = 2
    };
    
    explicit GraphViewResults(QWidget *parent = 0);
    virtual ~GraphViewResults();
    
    void setResultToShow(Result result, Variable variable);
    
    void setSettings(const ProjectSettings& settings);
    void setMCMCSettings(const MCMCSettings& mcmc, const QList<Chain>& chains);
    
    void updateChains(bool showAll, const QList<bool>& showChainList);
    void updateHPD(bool show, int threshold);
    
    void setMainColor(const QColor& color);
    void toggle(const QRect& geometry);
    
public slots:
    void setRange(float min, float max);
    void zoom(float min, float max);
    void showNumericalValues(bool show);
    
private slots:
    void saveAsImage();
    void imageToClipboard();
    void resultsToClipboard();
    virtual void saveGraphData() = 0;
    
protected:
    virtual void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* e);
    
    virtual void refresh() = 0;
    
signals:
    void unfoldToggled(bool toggled);
    void visibilityChanged(bool visible);
    
protected:
    GraphView* mGraph;
    
    Result mCurrentResult;
    Variable mCurrentVariable;
    
    QString mResults;
    bool mShowAllChains;
    QList<bool> mShowChainList;
    bool mShowHPD;
    int mThresholdHPD;
    
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
