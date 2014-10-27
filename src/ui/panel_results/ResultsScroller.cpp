#include "ResultsScroller.h"
#include "ScrollCompressor.h"
#include "GraphViewResults.h"
#include "ResultsMarker.h"

#include "GraphViewPhase.h"
#include "GraphViewEvent.h"
#include "GraphViewDate.h"

#include <QtWidgets>


ResultsScroller::ResultsScroller(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mEltHeight(100),
mEltHeightMin(30),
mEltHeightMax(300),
mOffset(0)
{
    mScrollBar = new QScrollBar(Qt::Vertical, this);
    mScrollBar->setRange(0, 0);
    mScrollBar->setSingleStep(1);
    mScrollBar->setPageStep(10000);
    mScrollBar->setTracking(true);
    
    mCompressor = new ScrollCompressor(this);
    
    connect(mScrollBar, SIGNAL(valueChanged(int)), this, SLOT(setPosition(int)));
    connect(mCompressor, SIGNAL(valueChanged(float)), this, SLOT(scale(float)));
}

ResultsScroller::~ResultsScroller()
{
    
}

void ResultsScroller::addElement(GraphViewResults* graph)
{
    graph->setParent(this);
    graph->setVisible(true);
    connect(graph, SIGNAL(visibilityChanged(bool)), this, SLOT(updateLayout()));
    mGraphs.append(graph);
    updateLayout();
}

void ResultsScroller::setRange(float min, float max)
{
    for(int i=0; i<mGraphs.size(); ++i)
    {
        mGraphs[i]->setRange(min, max);
    }
}

void ResultsScroller::zoom(float min, float max)
{
    for(int i=0; i<mGraphs.size(); ++i)
    {
        mGraphs[i]->zoom(min, max);
    }
}

void ResultsScroller::clear()
{
    for(int i=0; i<mGraphs.size(); ++i)
    {
        mGraphs[i]->setParent(0);
        delete mGraphs[i];
        mGraphs[i] = 0;
    }
    mGraphs.clear();
    updateLayout();
}

void ResultsScroller::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
    updateLayout();
}

void ResultsScroller::updateLayout()
{
    int sbe = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    int cmpH = 100;
    
    mScrollBar->setGeometry(width() - sbe, 0, sbe, height() - cmpH);
    mCompressor->setGeometry(width() - sbe, height() - cmpH, sbe, cmpH);
    
    //qDebug() << "------";
    int totalH = 0;
    for(int i=0; i<mGraphs.size(); ++i)
    {
        int h = mGraphs[i]->visible() ? mEltHeight : 0;
        mGraphs[i]->setGeometry(0, totalH - mOffset, width() - sbe, h);
        totalH += h;
        qDebug() << i << mGraphs[i]->geometry();
    }
    
    if(totalH <= height())
    {
        mScrollBar->setRange(0, 0);
    }
    else
    {
        mScrollBar->setRange(0, 10000);
        mScrollBar->setPageStep(10000 * height() / totalH);
        //qDebug() << 10000 * h / totalH;
    }
    update();
}

void ResultsScroller::scale(float prop)
{
    mEltHeight = mEltHeightMin + (mEltHeightMax - mEltHeightMin) * prop;
    setPosition(mScrollBar->value());
    updateLayout();
}

void ResultsScroller::setPosition(int pos)
{
    float range = (float)mScrollBar->maximum();
    float prop = (range > 0) ? (float)pos / range : 0;
    int totalH = mGraphs.size() * mEltHeight;
    int h = height();
    int offsetMax = totalH - h;
    mOffset = prop * offsetMax;
    updateLayout();
}

void ResultsScroller::wheelEvent(QWheelEvent* e)
{
    e->accept();
    qDebug() << e->delta();
    mScrollBar->setValue(mScrollBar->value() - e->delta());
}







void ResultsScroller::showPhasesHistos(bool showAlpha, bool showBeta, bool showPredict, bool showAllChains, const QList<bool>& showChainList, bool showHPD, int thresholdHPD)
{
    for(int i=0; i<mGraphs.size(); ++i)
    {
        GraphViewPhase* graph = dynamic_cast<GraphViewPhase*>(mGraphs[i]);
        if(graph)
        {
            graph->showHisto(showAlpha, showBeta, showPredict, showAllChains, showChainList, showHPD, thresholdHPD);
        }
    }
}
void ResultsScroller::showEventsHistos(bool showAllChains, const QList<bool>& showChainList, bool showHPD, int thresholdHPD)
{
    for(int i=0; i<mGraphs.size(); ++i)
    {
        GraphViewEvent* graph = dynamic_cast<GraphViewEvent*>(mGraphs[i]);
        if(graph)
        {
            graph->showHisto(showAllChains, showChainList, showHPD, thresholdHPD);
        }
    }
}
void ResultsScroller::showDataHistos(bool showTheta, bool showSigma, bool showDelta, bool showCalib, bool showAllChains, const QList<bool>& showChainList, bool showHPD, int thresholdHPD)
{
    for(int i=0; i<mGraphs.size(); ++i)
    {
        GraphViewDate* graph = dynamic_cast<GraphViewDate*>(mGraphs[i]);
        if(graph)
        {
            graph->showHisto(showTheta, showSigma, showDelta, showCalib, showAllChains, showChainList, showHPD, thresholdHPD);
        }
    }
}

void ResultsScroller::showPhasesTraces(bool showAlpha, bool showBeta, bool showPredict, const QList<bool>& showChainList)
{
    for(int i=0; i<mGraphs.size(); ++i)
    {
        GraphViewPhase* graph = dynamic_cast<GraphViewPhase*>(mGraphs[i]);
        if(graph)
        {
            graph->showTrace(showAlpha, showBeta, showPredict, showChainList);
        }
    }
}
void ResultsScroller::showEventsTraces(const QList<bool>& showChainList)
{
    for(int i=0; i<mGraphs.size(); ++i)
    {
        GraphViewEvent* graph = dynamic_cast<GraphViewEvent*>(mGraphs[i]);
        if(graph)
        {
            graph->showTrace(showChainList);
        }
    }
}
void ResultsScroller::showDataTraces(bool showTheta, bool showSigma, bool showDelta, const QList<bool>& showChainList)
{
    for(int i=0; i<mGraphs.size(); ++i)
    {
        GraphViewDate* graph = dynamic_cast<GraphViewDate*>(mGraphs[i]);
        if(graph)
        {
            graph->showTrace(showTheta, showSigma, showDelta, showChainList);
        }
    }
}

void ResultsScroller::showPhasesAccept(bool showAlpha, bool showBeta, bool showPredict, const QList<bool>& showChainList)
{
    for(int i=0; i<mGraphs.size(); ++i)
    {
        GraphViewPhase* graph = dynamic_cast<GraphViewPhase*>(mGraphs[i]);
        if(graph)
        {
            graph->showAccept(showAlpha, showBeta, showPredict, showChainList);
        }
    }
}
void ResultsScroller::showEventsAccept(const QList<bool>& showChainList)
{
    for(int i=0; i<mGraphs.size(); ++i)
    {
        GraphViewEvent* graph = dynamic_cast<GraphViewEvent*>(mGraphs[i]);
        if(graph)
        {
            graph->showAccept(showChainList);
        }
    }
}
void ResultsScroller::showDataAccept(bool showTheta, bool showSigma, bool showDelta, const QList<bool>& showChainList)
{
    for(int i=0; i<mGraphs.size(); ++i)
    {
        GraphViewDate* graph = dynamic_cast<GraphViewDate*>(mGraphs[i]);
        if(graph)
        {
            graph->showAccept(showTheta, showSigma, showDelta, showChainList);
        }
    }
}

