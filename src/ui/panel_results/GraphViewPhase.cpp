#include "GraphViewPhase.h"
#include "GraphView.h"
#include "Phase.h"
#include "Painting.h"
#include "StdUtilities.h"
#include "QtUtilities.h"
#include <QtWidgets>



#pragma mark Constructor / Destructor

GraphViewPhase::GraphViewPhase(QWidget *parent):GraphViewResults(parent),
mPhase(0)
{
    mGraph->setBackgroundColor(QColor(230, 230, 230));
}

GraphViewPhase::~GraphViewPhase()
{
    mPhase = 0;
}

void GraphViewPhase::setPhase(Phase* phase)
{
    if(phase)
        mPhase = phase;
    update();
}

void GraphViewPhase::saveGraphData()
{
    
}

void GraphViewPhase::paintEvent(QPaintEvent* e)
{
    GraphViewResults::paintEvent(e);
    
    QPainter p(this);
    
    if(mPhase)
    {
        QColor backCol = mPhase->mColor;
        QColor foreCol = getContrastedColor(backCol);
        
        QRect topRect(0, 0, mGraphLeft, mLineH);
        p.fillRect(topRect.adjusted(1, 1, -1, 0), backCol);
        
        p.setPen(foreCol);
        QFont font;
        font.setPointSizeF(pointSize(11));
        p.setFont(font);
        p.drawText(topRect.adjusted(mMargin, 0, -mMargin, 0),
                   Qt::AlignVCenter | Qt::AlignLeft,
                   tr("Phase") + " : " + mPhase->mName);
    }
}

void GraphViewPhase::refresh()
{
    mGraph->removeAllCurves();
    mGraph->removeAllZones();
    
    if(mPhase)
    {
        QColor color = mPhase->mColor;
        
        QString results;
        results += "----------------------------------\n";
        results += tr("PHASE BEGIN");
        results += "----------------------------------\n";
        results += mPhase->mAlpha.resultsText(mThresholdHPD);
        results += "----------------------------------\n";
        results += tr("PHASE END");
        results += "----------------------------------\n";
        results += mPhase->mBeta.resultsText(mThresholdHPD);
        
        setNumericalResults(results);
        
        if(mCurrentResult == eHisto)
        {
            mGraph->setRangeX(mSettings.mTmin, mSettings.mTmax);
            mGraph->setRangeY(0, 1);
            
            if(mShowAllChains)
            {
                GraphCurve curveAlpha;
                curveAlpha.mName = "alpha full";
                curveAlpha.mPen.setColor(color);
                curveAlpha.mData = normalize_map(mPhase->mAlpha.fullHisto());
                mGraph->addCurve(curveAlpha);
                
                GraphCurve curveBeta;
                curveBeta.mName = QString("beta full");
                curveBeta.mPen.setColor(color);
                curveBeta.mData = normalize_map(mPhase->mBeta.fullHisto());
                mGraph->addCurve(curveBeta);
                
                if(mShowHPD)
                {
                    GraphCurve curveAlphaHPD;
                    curveAlphaHPD.mName = "alpha HPD full";
                    curveAlphaHPD.mPen.setColor(color);
                    curveAlphaHPD.mFillUnder = true;
                    curveAlphaHPD.mData = normalize_map(mPhase->mAlpha.generateFullHPD(mThresholdHPD));
                    mGraph->addCurve(curveAlphaHPD);
                    
                    GraphCurve curveBetaHPD;
                    curveBetaHPD.mName = "beta HPD full";
                    curveBetaHPD.mPen.setColor(color);
                    curveBetaHPD.mFillUnder = true;
                    curveBetaHPD.mData = normalize_map(mPhase->mBeta.generateFullHPD(mThresholdHPD));
                    mGraph->addCurve(curveBetaHPD);
                }
            }
            for(int i=0; i<mShowChainList.size(); ++i)
            {
                if(mShowChainList[i])
                {
                    QColor col = Painting::chainColors[i];
                    
                    GraphCurve curveAlphaChain;
                    curveAlphaChain.mName = QString("alpha chain " + QString::number(i));
                    curveAlphaChain.mPen.setColor(col);
                    curveAlphaChain.mData = normalize_map(mPhase->mAlpha.histoForChain(i));
                    mGraph->addCurve(curveAlphaChain);
                    
                    GraphCurve curveBetaChain;
                    curveBetaChain.mName = QString("beta chain " + QString::number(i));
                    curveBetaChain.mPen.setColor(col);
                    curveBetaChain.mData = normalize_map(mPhase->mBeta.histoForChain(i));
                    mGraph->addCurve(curveBetaChain);
                    
                    if(mShowHPD)
                    {
                        GraphCurve curveAlphaChainHPD;
                        curveAlphaChainHPD.mName = QString("alpha HPD chain " + QString::number(i));
                        curveAlphaChainHPD.mPen.setColor(col);
                        curveAlphaChainHPD.mFillUnder = true;
                        curveAlphaChainHPD.mData = normalize_map(mPhase->mAlpha.generateHPDForChain(i, mThresholdHPD));
                        mGraph->addCurve(curveAlphaChainHPD);
                        
                        GraphCurve curveBetaChainHPD;
                        curveBetaChainHPD.mName = QString("beta HPD chain " + QString::number(i));
                        curveBetaChainHPD.mPen.setColor(col);
                        curveBetaChainHPD.mFillUnder = true;
                        curveBetaChainHPD.mData = normalize_map(mPhase->mBeta.generateHPDForChain(i, mThresholdHPD));
                        mGraph->addCurve(curveBetaChainHPD);
                    }
                }
            }
        }
        else if(mCurrentResult == eTrace)
        {
            int chainIdx = -1;
            for(int i=0; i<mShowChainList.size(); ++i)
                if(mShowChainList[i])
                    chainIdx = i;
            
            if(chainIdx != -1)
            {
                Chain& chain = mChains[chainIdx];
                mGraph->setRangeX(0, chain.mNumBurnIter + chain.mNumBatchIter * chain.mBatchIndex + chain.mNumRunIter);
                
                QColor col = Painting::chainColors[chainIdx];
                
                GraphCurve curveAlpha;
                curveAlpha.mName = QString("alpha trace chain " + QString::number(chainIdx));
                curveAlpha.mData = mPhase->mAlpha.traceForChain(mChains, chainIdx);
                curveAlpha.mPen.setColor(col);
                mGraph->addCurve(curveAlpha);
                
                GraphCurve curveBeta;
                curveBeta.mName = QString("beta trace chain " + QString::number(chainIdx));
                curveBeta.mData = mPhase->mBeta.traceForChain(mChains, chainIdx);
                curveBeta.mPen.setColor(col);
                mGraph->addCurve(curveBeta);
                
                float min = qMin(map_min_value(curveBeta.mData), map_min_value(curveAlpha.mData));
                float max = qMax(map_max_value(curveBeta.mData), map_max_value(curveAlpha.mData));
                
                mGraph->setRangeY(min, max);
            }
        }
        else if(mCurrentResult == eAccept)
        {
            mGraph->removeAllCurves();
        }
        else if(mCurrentResult == eCorrel)
        {
            mGraph->removeAllCurves();
        }
    }
}
