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
    {
        mPhase = phase;
        mTitle = tr("Phase") + " : " + mPhase->mName;
    }
    update();
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
        p.setPen(backCol);
        p.setBrush(backCol);
        p.drawRect(topRect);
        
        p.setPen(Qt::black);
        p.drawLine(0, height(), mGraphLeft, height());
        
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
    mGraph->clearInfos();
    setNumericalResults("");
    
    if(mPhase)
    {
        QColor color = mPhase->mColor;
        
        if(mCurrentResult == eHisto && mCurrentVariable == eTheta)
        {
            QString results;
            results += mTitle + "\n";
            results += "----------------------------------\n";
            //results += "Period : [" + QString::number(mPhase->mAlpha.mResults.analysis.mean, 'f', 0) + ", " + QString::number(mPhase->mBeta.mResults.analysis.mean, 'f', 0) + "]\n";
            results += "Duration credibility (95%) : " + mPhase->mDurationCredibility + "\n";
            results += "----------------------------------\n";
            results += tr("PHASE BEGIN") + "\n";
            results += mPhase->mAlpha.resultsText();
            results += "----------------------------------\n";
            results += tr("PHASE END") + "\n";
            results += mPhase->mBeta.resultsText();
            
            setNumericalResults(results);
            
            mGraph->setRangeX(mSettings.mTmin, mSettings.mTmax);
            mGraph->setRangeY(0, 0.0001f);
            
            if(mShowAllChains)
            {
                QColor alphaCol = color;
                QColor betaCol = color;
                //alphaCol.setRed(color.blue());
                //alphaCol.setBlue(color.red());
                
                GraphCurve curveAlpha;
                curveAlpha.mName = "alpha full";
                curveAlpha.mPen.setColor(alphaCol);
                curveAlpha.mIsHisto = false;
                curveAlpha.mData = equal_areas(mPhase->mAlpha.fullHisto(), 1.f);
                mGraph->addCurve(curveAlpha);
                
                float yMax = 1.1f * map_max_value(curveAlpha.mData);
                mGraph->setRangeY(0, qMax(mGraph->maximumY(), yMax));
                
                GraphCurve curveBeta;
                curveBeta.mName = QString("beta full");
                curveBeta.mPen.setColor(betaCol);
                curveBeta.mIsHisto = false;
                curveBeta.mData = equal_areas(mPhase->mBeta.fullHisto(), 1.f);
                mGraph->addCurve(curveBeta);
                
                yMax = 1.1f * map_max_value(curveBeta.mData);
                mGraph->setRangeY(0, qMax(mGraph->maximumY(), yMax));
                
                // HPD
                
                GraphCurve curveAlphaHPD;
                curveAlphaHPD.mName = "alpha HPD full";
                curveAlphaHPD.mPen.setColor(alphaCol);
                curveAlphaHPD.mFillUnder = true;
                curveAlphaHPD.mIsHisto = false;
                curveAlphaHPD.mIsRectFromZero = true;
                curveAlphaHPD.mData = equal_areas(mPhase->mAlpha.mHPD, mThresholdHPD / 100.f);
                mGraph->addCurve(curveAlphaHPD);
                
                GraphCurve curveBetaHPD;
                curveBetaHPD.mName = "beta HPD full";
                curveBetaHPD.mPen.setColor(color);
                curveBetaHPD.mFillUnder = true;
                curveBetaHPD.mIsHisto = false;
                curveBetaHPD.mIsRectFromZero = true;
                curveBetaHPD.mData = equal_areas(mPhase->mBeta.mHPD, mThresholdHPD / 100.f);
                mGraph->addCurve(curveBetaHPD);
                
                if(mShowRawResults)
                {
                    GraphCurve curveRawAlpha;
                    curveRawAlpha.mName = "raw alpha";
                    curveRawAlpha.mPen.setColor(Qt::red);
                    curveRawAlpha.mData = equal_areas(mPhase->mAlpha.fullRawHisto(), 1.f);
                    curveRawAlpha.mIsHisto = true;
                    mGraph->addCurve(curveRawAlpha);
                    
                    yMax = 1.1f * map_max_value(curveRawAlpha.mData);
                    mGraph->setRangeY(0, qMax(mGraph->maximumY(), yMax));
                    
                    GraphCurve curveRawBeta;
                    curveRawBeta.mName = "raw beta";
                    curveRawBeta.mPen.setColor(Qt::red);
                    curveRawBeta.mData = equal_areas(mPhase->mBeta.fullRawHisto(), 1.f);
                    curveRawBeta.mIsHisto = true;
                    mGraph->addCurve(curveRawBeta);
                    
                    yMax = 1.1f * map_max_value(curveRawBeta.mData);
                    mGraph->setRangeY(0, qMax(mGraph->maximumY(), yMax));
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
                    curveAlphaChain.mIsHisto = false;
                    curveAlphaChain.mData = equal_areas(mPhase->mAlpha.histoForChain(i), 1.f);
                    mGraph->addCurve(curveAlphaChain);
                    
                    float yMax = 1.1f * map_max_value(curveAlphaChain.mData);
                    mGraph->setRangeY(0, qMax(mGraph->maximumY(), yMax));
                    
                    GraphCurve curveBetaChain;
                    curveBetaChain.mName = QString("beta chain " + QString::number(i));
                    curveBetaChain.mPen.setColor(col);
                    curveBetaChain.mIsHisto = false;
                    curveBetaChain.mData = equal_areas(mPhase->mBeta.histoForChain(i), 1.f);
                    mGraph->addCurve(curveBetaChain);
                    
                    yMax = 1.1f * map_max_value(curveBetaChain.mData);
                    mGraph->setRangeY(0, qMax(mGraph->maximumY(), yMax));
                }
            }
        }
        else if(mCurrentResult == eTrace && mCurrentVariable == eTheta)
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
                curveAlpha.mData = mPhase->mAlpha.fullTraceForChain(mChains, chainIdx);
                curveAlpha.mPen.setColor(col);
                mGraph->addCurve(curveAlpha);
                
                GraphCurve curveBeta;
                curveBeta.mName = QString("beta trace chain " + QString::number(chainIdx));
                curveBeta.mData = mPhase->mBeta.fullTraceForChain(mChains, chainIdx);
                curveBeta.mPen.setColor(col);
                mGraph->addCurve(curveBeta);
                
                float min = qMin(map_min_value(curveBeta.mData), map_min_value(curveAlpha.mData));
                float max = qMax(map_max_value(curveBeta.mData), map_max_value(curveAlpha.mData));
                
                mGraph->setRangeY(min, max);
            }
        }
    }
}
