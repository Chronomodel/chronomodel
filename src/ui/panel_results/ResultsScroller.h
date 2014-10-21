#ifndef ResultsScroller_H
#define ResultsScroller_H

#include <QWidget>

class QScrollBar;
class QVBoxLayout;
class ScrollCompressor;
class GraphViewResults;

class ResultsScroller: public QWidget
{
    Q_OBJECT
public:
    ResultsScroller(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~ResultsScroller();
    
    void addElement(GraphViewResults* graph);
    void clear();
    
    // ----------------------------------------------------------------
    
    void showPhasesHistos(bool showAlpha, bool showBeta, bool showPredict, bool showAllChains, const QList<bool>& showChainList, bool showHPD, int thresholdHPD);
    void showEventsHistos(bool showAllChains, const QList<bool>& showChainList, bool showHPD, int thresholdHPD);
    void showDataHistos(bool showTheta, bool showSigma, bool showDelta, bool showCalib, bool showAllChains, const QList<bool>& showChainList, bool showHPD, int thresholdHPD);
    
    void showPhasesTraces(bool showAlpha, bool showBeta, bool showPredict, const QList<bool>& showChainList);
    void showEventsTraces(const QList<bool>& showChainList);
    void showDataTraces(bool showTheta, bool showSigma, bool showDelta, const QList<bool>& showChainList);
    
    void showPhasesAccept(bool showAlpha, bool showBeta, bool showPredict, const QList<bool>& showChainList);
    void showEventsAccept(const QList<bool>& showChainList);
    void showDataAccept(bool showTheta, bool showSigma, bool showDelta, const QList<bool>& showChainList);
    
    // ----------------------------------------------------------------
    
public slots:
    void setRange(float min, float max);
    void zoom(float min, float max);
    
protected:
    void resizeEvent(QResizeEvent* e);
    void wheelEvent(QWheelEvent* e);
    
protected slots:
    void updateLayout();
    
private slots:
    void scale(float prop);
    void setPosition(int pos);
    
private:
    QList<GraphViewResults*> mGraphs;
    QScrollBar* mScrollBar;
    ScrollCompressor* mCompressor;
    
    int mEltHeight;
    int mEltHeightMin;
    int mEltHeightMax;
    
    int mOffset;
};

#endif
