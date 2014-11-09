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

void GraphViewPhase::setVariablesToShow(bool showAlpha, bool showBeta, bool showTau)
{
    mShowAlpha = showAlpha;
    mShowBeta = showBeta;
    mShowTau = showTau;
    refresh();
}

void GraphViewPhase::refresh()
{
    mGraph->removeAllCurves();
    mGraph->removeAllZones();
    
    if(mPhase)
    {
        QColor color = mPhase->mColor;
        
        if(mCurrentResult == eHisto)
        {
            mGraph->setRangeX(mSettings.mTmin, mSettings.mTmax);
            mGraph->setRangeY(0, 1);
            
            if(mShowAlpha)
            {
                if(mShowAllChains)
                {
                    GraphCurve curve;
                    curve.mName = "alpha full";
                    curve.mPen.setColor(color);
                    curve.mData = normalize_map(mPhase->mAlpha.fullHisto());
                    mGraph->addCurve(curve);
                    
                    if(mShowHPD)
                    {
                        GraphCurve curveHPD;
                        curveHPD.mName = "alpha HPD full";
                        curveHPD.mPen.setColor(color);
                        curveHPD.mFillUnder = true;
                        curveHPD.mData = normalize_map(mPhase->mAlpha.generateFullHPD(mThresholdHPD));
                        mGraph->addCurve(curveHPD);
                    }
                }
                for(int i=0; i<mShowChainList.size(); ++i)
                {
                    if(mShowChainList[i])
                    {
                        QColor col = Painting::chainColors[i];
                        
                        GraphCurve curveChain;
                        curveChain.mName = QString("alpha chain " + QString::number(i));
                        curveChain.mPen.setColor(col);
                        curveChain.mData = normalize_map(mPhase->mAlpha.histoForChain(i));
                        mGraph->addCurve(curveChain);
                        
                        if(mShowHPD)
                        {
                            GraphCurve curveChainHPD;
                            curveChainHPD.mName = QString("alpha HPD chain " + QString::number(i));
                            curveChainHPD.mPen.setColor(col);
                            curveChainHPD.mFillUnder = true;
                            curveChainHPD.mData = normalize_map(mPhase->mAlpha.generateHPDForChain(i, mThresholdHPD));
                            mGraph->addCurve(curveChainHPD);
                        }
                    }
                }
            }
            if(mShowBeta)
            {
                if(mShowAllChains)
                {
                    GraphCurve curve;
                    curve.mName = QString("beta full");
                    curve.mPen.setColor(color);
                    curve.mData = normalize_map(mPhase->mBeta.fullHisto());
                    mGraph->addCurve(curve);
                    
                    if(mShowHPD)
                    {
                        GraphCurve curveHPD;
                        curveHPD.mName = "beta HPD full";
                        curveHPD.mPen.setColor(color);
                        curveHPD.mFillUnder = true;
                        curveHPD.mData = normalize_map(mPhase->mBeta.generateFullHPD(mThresholdHPD));
                        mGraph->addCurve(curveHPD);
                    }
                }
                for(int i=0; i<mShowChainList.size(); ++i)
                {
                    if(mShowChainList[i])
                    {
                        QColor col = Painting::chainColors[i];
                        
                        GraphCurve curveChain;
                        curveChain.mName = QString("beta chain " + QString::number(i));
                        curveChain.mPen.setColor(col);
                        curveChain.mData = normalize_map(mPhase->mBeta.histoForChain(i));
                        mGraph->addCurve(curveChain);
                        
                        if(mShowHPD)
                        {
                            GraphCurve curveChainHPD;
                            curveChainHPD.mName = QString("beta HPD chain " + QString::number(i));
                            curveChainHPD.mPen.setColor(col);
                            curveChainHPD.mFillUnder = true;
                            curveChainHPD.mData = normalize_map(mPhase->mBeta.generateHPDForChain(i, mThresholdHPD));
                            mGraph->addCurve(curveChainHPD);
                        }
                    }
                }
            }
            if(mShowTau)
            {
                if(mShowAllChains)
                {
                    GraphCurve curve;
                    curve.mName = QString("predict full");
                    curve.mPen.setColor(color);
                    curve.mData = normalize_map(mPhase->mTau.fullHisto());
                    mGraph->addCurve(curve);
                    
                    if(mShowHPD)
                    {
                        GraphCurve curveHPD;
                        curveHPD.mName = "predict HPD full";
                        curveHPD.mPen.setColor(color);
                        curveHPD.mFillUnder = true;
                        curveHPD.mData = normalize_map(mPhase->mTau.generateFullHPD(mThresholdHPD));
                        mGraph->addCurve(curveHPD);
                    }
                }
                for(int i=0; i<mShowChainList.size(); ++i)
                {
                    if(mShowChainList[i])
                    {
                        QColor col = Painting::chainColors[i];
                        
                        GraphCurve curveChain;
                        curveChain.mName = QString("predict chain " + QString::number(i));
                        curveChain.mPen.setColor(col);
                        curveChain.mData = normalize_map(mPhase->mTau.histoForChain(i));
                        mGraph->addCurve(curveChain);
                        
                        if(mShowHPD)
                        {
                            GraphCurve curveChainHPD;
                            curveChainHPD.mName = QString("predict HPD chain " + QString::number(i));
                            curveChainHPD.mPen.setColor(col);
                            curveChainHPD.mFillUnder = true;
                            curveChainHPD.mData = normalize_map(mPhase->mTau.generateHPDForChain(i, mThresholdHPD));
                            mGraph->addCurve(curveChainHPD);
                        }
                    }
                }
            }
        }
        else if(mCurrentResult == eTrace)
        {
            mGraph->setRangeX(0, mMCMCSettings.mNumRunIter);
            
            float min = 999999;
            float max = -999999;
            
            if(mShowAlpha)
            {
                for(int i=0; i<mShowChainList.size(); ++i)
                {
                    if(mShowChainList[i])
                    {
                        QColor col = Painting::chainColors[i];
                        
                        GraphCurve curve;
                        curve.mName = QString("alpha trace chain " + QString::number(i));
                        curve.mUseVectorData = true;
                        curve.mDataVector = mPhase->mAlpha.traceForChain(i, mShowChainList.size());
                        curve.mPen.setColor(col);
                        mGraph->addCurve(curve);
                        
                        min = qMin(vector_min_value(curve.mDataVector), min);
                        max = qMax(vector_max_value(curve.mDataVector), max);
                    }
                }
            }
            if(mShowBeta)
            {
                for(int i=0; i<mShowChainList.size(); ++i)
                {
                    if(mShowChainList[i])
                    {
                        QColor col = Painting::chainColors[i];
                        
                        GraphCurve curve;
                        curve.mName = QString("beta trace chain " + QString::number(i));
                        curve.mUseVectorData = true;
                        curve.mDataVector = mPhase->mBeta.traceForChain(i, mShowChainList.size());
                        curve.mPen.setColor(col);
                        mGraph->addCurve(curve);
                        
                        min = qMin(vector_min_value(curve.mDataVector), min);
                        max = qMax(vector_max_value(curve.mDataVector), max);
                    }
                }
            }
            if(mShowTau)
            {
                for(int i=0; i<mShowChainList.size(); ++i)
                {
                    if(mShowChainList[i])
                    {
                        QColor col = Painting::chainColors[i];
                        
                        GraphCurve curve;
                        curve.mName = QString("predict trace chain " + QString::number(i));
                        curve.mUseVectorData = true;
                        curve.mDataVector = mPhase->mTau.traceForChain(i, mShowChainList.size());
                        curve.mPen.setColor(col);
                        mGraph->addCurve(curve);
                        
                        min = qMin(vector_min_value(curve.mDataVector), min);
                        max = qMax(vector_max_value(curve.mDataVector), max);
                    }
                }
            }
            if(min < max)
                mGraph->setRangeY(min, max);
            else
                mGraph->setRangeY(0, 1);
        }
        else if(mCurrentResult == eAccept)
        {
            mGraph->setRangeX(0, mMCMCSettings.mNumRunIter);
            mGraph->setRangeY(0, 100);
        }
        else if(mCurrentResult == eCorrel)
        {
            
        }
    }
}
