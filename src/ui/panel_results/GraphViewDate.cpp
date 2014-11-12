#include "GraphViewDate.h"
#include "GraphView.h"
#include "Date.h"
#include "Event.h"
#include "Painting.h"
#include "StdUtilities.h"
#include "QtUtilities.h"
#include <QtWidgets>



#pragma mark Constructor / Destructor

GraphViewDate::GraphViewDate(QWidget *parent):GraphViewResults(parent),
mDate(0),
mColor(Qt::blue)
{
    //setMainColor(QColor(100, 120, 100));
    setMainColor(QColor(150, 150, 150));
}

GraphViewDate::~GraphViewDate()
{
    mDate = 0;
}

void GraphViewDate::setDate(Date* date)
{
    if(date)
        mDate = date;
    update();
}

void GraphViewDate::setColor(const QColor& color)
{
    mColor = color;
    update();
}

void GraphViewDate::saveGraphData()
{
    
}

void GraphViewDate::paintEvent(QPaintEvent* e)
{
    GraphViewResults::paintEvent(e);
    
    QPainter p(this);
    
    if(mDate)
    {
        QColor backCol = mColor;
        QColor foreCol = getContrastedColor(backCol);
        
        QRect topRect(0, 0, mGraphLeft, mLineH);
        p.fillRect(topRect.adjusted(1, 1, -1, 0), backCol);
        
        p.setPen(foreCol);
        QFont font;
        font.setPointSizeF(pointSize(11));
        p.setFont(font);
        p.drawText(topRect.adjusted(mMargin, 0, -mMargin, 0),
                   Qt::AlignVCenter | Qt::AlignLeft,
                   tr("Data") + " : " + mDate->mName);
    }
}

void GraphViewDate::refresh()
{
    mGraph->removeAllCurves();
    mGraph->removeAllZones();
    
    if(mDate)
    {
        QColor color = mColor;
        setNumericalResults(mDate->mTheta.resultsText());
        
        if(mCurrentResult == eHisto)
        {
            mGraph->setRangeY(0, 0.0001f);
            
            if(mCurrentVariable == eTheta)
                mGraph->setRangeX(mSettings.mTmin, mSettings.mTmax);
            else if(mCurrentVariable == eSigma || mCurrentVariable == eDelta)
                mGraph->setRangeX(0, mSettings.mTmax - mSettings.mTmin);
            
            MHVariable* variable = &(mDate->mTheta);
            if(mCurrentVariable == eTheta) variable = &(mDate->mTheta);
            else if(mCurrentVariable == eSigma) variable = &(mDate->mSigma);
            else if(mCurrentVariable == eDelta) variable = &(mDate->mDelta);
            
            if(mShowCalib && mCurrentVariable == eTheta)
            {
                GraphCurve curve;
                curve.mName = "calibration";
                //curve.mData = normalize_map(mDate->mCalibration);
                curve.mData = equal_areas(mDate->mCalibration, 100.f);
                curve.mPen.setColor(QColor(0, 0, 0));
                curve.mFillUnder = false;
                mGraph->addCurve(curve);
                
                float yMax = 1.1f * map_max_value(curve.mData);
                mGraph->setRangeY(0, qMax(mGraph->maximumY(), yMax));
            }
            
            if(mShowAllChains)
            {
                GraphCurve curve;
                curve.mName = "histo full";
                curve.mData = equal_areas(variable->fullHisto(), 100.f);
                curve.mPen.setColor(color);
                mGraph->addCurve(curve);
                
                float yMax = 1.1f * map_max_value(curve.mData);
                mGraph->setRangeY(0, qMax(mGraph->maximumY(), yMax));
                
                if(mShowHPD)
                {
                    GraphCurve curveHPD;
                    curveHPD.mName = "histo HPD full";
                    curveHPD.mData = equal_areas(variable->generateFullHPD(mThresholdHPD), mThresholdHPD);
                    curveHPD.mPen.setColor(color);
                    curveHPD.mFillUnder = true;
                    mGraph->addCurve(curveHPD);
                }
            }
            for(int i=0; i<mShowChainList.size(); ++i)
            {
                if(mShowChainList[i])
                {
                    QColor col = Painting::chainColors[i];
                    
                    GraphCurve curve;
                    curve.mName = QString("histo chain " + QString::number(i));
                    curve.mData = equal_areas(variable->histoForChain(i), 100.f);
                    curve.mPen.setColor(col);
                    mGraph->addCurve(curve);
                    
                    float yMax = 1.1f * map_max_value(curve.mData);
                    mGraph->setRangeY(0, qMax(mGraph->maximumY(), yMax));
                    
                    if(mShowHPD)
                    {
                        GraphCurve curveHPD;
                        curveHPD.mName = QString("histo HPD chain " + QString::number(i));
                        curveHPD.mData = equal_areas(variable->generateHPDForChain(i, mThresholdHPD), mThresholdHPD);
                        curveHPD.mPen.setColor(col);
                        curveHPD.mFillUnder = true;
                        mGraph->addCurve(curveHPD);
                    }
                }
            }
        }
        if(mCurrentResult == eTrace)
        {
            int chainIdx = -1;
            for(int i=0; i<mShowChainList.size(); ++i)
                if(mShowChainList[i])
                    chainIdx = i;
            
            if(chainIdx != -1)
            {
                Chain& chain = mChains[chainIdx];
                mGraph->setRangeX(0, chain.mNumBurnIter + chain.mNumBatchIter * chain.mBatchIndex + chain.mNumRunIter);
                
                MHVariable* variable = &(mDate->mTheta);
                if(mCurrentVariable == eTheta) variable = &(mDate->mTheta);
                else if(mCurrentVariable == eSigma) variable = &(mDate->mSigma);
                else if(mCurrentVariable == eDelta) variable = &(mDate->mDelta);
                
                GraphCurve curve;
                curve.mName = QString("trace chain " + QString::number(chainIdx));
                curve.mData = variable->traceForChain(mChains, chainIdx);
                curve.mPen.setColor(Painting::chainColors[chainIdx]);
                curve.mIsHisto = false;
                mGraph->addCurve(curve);
                
                float min = map_min_value(curve.mData);
                float max = map_max_value(curve.mData);
                mGraph->setRangeY(floorf(min), ceilf(max));
            }
        }
        else if(mCurrentResult == eAccept)
        {
            int chainIdx = -1;
            for(int i=0; i<mShowChainList.size(); ++i)
                if(mShowChainList[i])
                    chainIdx = i;
            
            if(chainIdx != -1)
            {
                Chain& chain = mChains[chainIdx];
                mGraph->setRangeX(0, chain.mNumBurnIter + chain.mNumBatchIter * chain.mBatchIndex + chain.mNumRunIter);
                mGraph->setRangeY(0, 100);
                
                MHVariable* variable = &(mDate->mTheta);
                if(mCurrentVariable == eTheta) variable = &(mDate->mTheta);
                else if(mCurrentVariable == eSigma) variable = &(mDate->mSigma);
                else if(mCurrentVariable == eDelta) variable = &(mDate->mDelta);
                
                GraphCurve curveTarget;
                curveTarget.mName = "target";
                curveTarget.mIsHorizontalLine = true;
                curveTarget.mHorizontalValue = 44;
                curveTarget.mPen.setStyle(Qt::DashLine);
                curveTarget.mPen.setColor(QColor(180, 10, 20));
                mGraph->addCurve(curveTarget);
                
                GraphCurve curve;
                curve.mName = QString("accept history chain " + QString::number(chainIdx));
                curve.mData = variable->acceptationForChain(mChains, chainIdx);
                curve.mPen.setColor(Painting::chainColors[chainIdx]);
                curve.mIsHisto = false;
                mGraph->addCurve(curve);
            }
           
        }
        else if(mCurrentResult == eCorrel)
        {
            int chainIdx = -1;
            for(int i=0; i<mShowChainList.size(); ++i)
                if(mShowChainList[i])
                    chainIdx = i;
            
            if(chainIdx != -1)
            {
                MHVariable* variable = &(mDate->mTheta);
                if(mCurrentVariable == eTheta) variable = &(mDate->mTheta);
                else if(mCurrentVariable == eSigma) variable = &(mDate->mSigma);
                else if(mCurrentVariable == eDelta) variable = &(mDate->mDelta);
                
                GraphCurve curve;
                curve.mName = QString("correlation chain " + QString::number(chainIdx));
                curve.mDataVector = variable->correlationForChain(chainIdx);
                curve.mUseVectorData = true;
                curve.mPen.setColor(Painting::chainColors[chainIdx]);
                curve.mIsHisto = false;
                mGraph->addCurve(curve);
                
                mGraph->setRangeX(0, 100);
                mGraph->setRangeY(vector_min_value(curve.mDataVector),
                                  vector_max_value(curve.mDataVector));
            }
        }
    }
}


