#include "GraphViewDate.h"
#include "GraphView.h"
#include "Date.h"
#include "Event.h"
#include "Painting.h"
#include "StdUtilities.h"
#include "QtUtilities.h"
#include "ModelUtilities.h"
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
    {
        mDate = date;
        mTitle = tr("Data") + " : " + mDate->mName;
    }
    update();
}

void GraphViewDate::setColor(const QColor& color)
{
    mColor = color;
    update();
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
                   tr("Data") + " : " + mDate->mName);
    }
}

void GraphViewDate::refresh()
{
    mGraph->removeAllCurves();
    mGraph->removeAllZones();
    mGraph->clearInfos();
    //setNumericalResults("");
    
    mGraph->autoAdjustYScale(mCurrentResult == eTrace);
    
    if(mDate)
    {
        QColor color = mColor;
        
        if(mCurrentResult == eHisto)
        {
            mGraph->setRangeY(0, 0.0001f);
            
            if(mCurrentVariable == eTheta)
                mGraph->setRangeX(mSettings.mTmin, mSettings.mTmax);
            else if(mCurrentVariable == eSigma)
                mGraph->setRangeX(0, mSettings.mTmax - mSettings.mTmin);
            
            MHVariable* variable = &(mDate->mTheta);
            if(mCurrentVariable == eTheta) variable = &(mDate->mTheta);
            else if(mCurrentVariable == eSigma) variable = &(mDate->mSigma);
            
            setNumericalResults(mTitle + "\n" + variable->resultsText());
            
            if(mShowCalib && mCurrentVariable == eTheta)
            {
                GraphCurve curve;
                curve.mName = "calibration";
                curve.mData = equal_areas(mDate->getCalibMap(), 1.f);
                curve.mPen.setColor(QColor(120, 120, 120));
                curve.mFillUnder = false;
                curve.mIsHisto = false;
                curve.mIsRectFromZero = true; // for typo. calibs., invisible for others!
                mGraph->addCurve(curve);
                
                double yMax = 1.1f * map_max_value(curve.mData);
                mGraph->setRangeY(0, qMax(mGraph->maximumY(), yMax));
            }
            
            if(mShowWiggle && mCurrentVariable == eTheta)
            {
                GraphCurve curve;
                curve.mName = "wiggle";
                curve.mData = equal_areas(mDate->mWiggle.fullHisto(), 1.f);
                curve.mPen.setColor(color);
                curve.mPen.setStyle(Qt::DashLine);
                curve.mFillUnder = false;
                curve.mIsHisto = false;
                mGraph->addCurve(curve);
                
                double yMax = 1.1f * map_max_value(curve.mData);
                mGraph->setRangeY(0, qMax(mGraph->maximumY(), yMax));
            }
            
            if(mShowAllChains)
            {
                if(mShowRawResults)
                {
                    GraphCurve curveRaw;
                    curveRaw.mName = "raw histo full";
                    curveRaw.mPen.setColor(Qt::red);
                    curveRaw.mData = equal_areas(variable->fullRawHisto(), 1.f);
                    curveRaw.mIsHisto = true;
                    mGraph->addCurve(curveRaw);
                    
                    double yMax2 = 1.1f * map_max_value(curveRaw.mData);
                    mGraph->setRangeY(0, qMax(mGraph->maximumY(), yMax2));
                }
                
                GraphCurve curve;
                curve.mName = "histo full";
                curve.mData = equal_areas(variable->fullHisto(), 1.f);
                curve.mPen.setColor(color);
                curve.mIsHisto = false;
                mGraph->addCurve(curve);
                
                double yMax = 1.1f * map_max_value(curve.mData);
                mGraph->setRangeY(0, qMax(mGraph->maximumY(), yMax));
                
                if(mCurrentVariable != GraphViewResults::eSigma)
                {
                    GraphCurve curveHPD;
                    curveHPD.mName = "histo HPD full";
                    curveHPD.mData = equal_areas(variable->mHPD, mThresholdHPD/100.f);
                    curveHPD.mPen.setColor(color);
                    curveHPD.mFillUnder = true;
                    curveHPD.mIsHisto = false;
                    curveHPD.mIsRectFromZero = true;
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
                    curve.mData = equal_areas(variable->histoForChain(i), 1.f);
                    curve.mPen.setColor(col);
                    curve.mIsHisto = false;
                    mGraph->addCurve(curve);
                    
                    double yMax = 1.1f * map_max_value(curve.mData);
                    mGraph->setRangeY(0, qMax(mGraph->maximumY(), yMax));
                }
            }
            if(mShowAllChains && mShowHPD && mCurrentVariable != GraphViewResults::eSigma)
            {
                GraphCurve curveCred;
                curveCred.mName = "credibility full";
                curveCred.mSections.append(variable->mCredibility);
                curveCred.mHorizontalValue = mGraph->maximumY();
                curveCred.mPen.setColor(color);
                curveCred.mPen.setWidth(5);
                curveCred.mIsHorizontalSections = true;
                mGraph->addCurve(curveCred);
            }
            if(mCurrentVariable == GraphViewResults::eSigma)
            {
                mGraph->autoAdjustYScale(true);
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
                mGraph->setRangeX(0, chain.mNumBurnIter + chain.mNumBatchIter * chain.mBatchIndex + chain.mNumRunIter / chain.mThinningInterval);
                
                MHVariable* variable = &(mDate->mTheta);
                if(mCurrentVariable == eTheta) variable = &(mDate->mTheta);
                else if(mCurrentVariable == eSigma) variable = &(mDate->mSigma);
                
                GraphCurve curve;
                curve.mUseVectorData = true;
                curve.mName = QString("trace chain " + QString::number(chainIdx));
                curve.mDataVector = variable->fullTraceForChain(mChains, chainIdx);
                curve.mPen.setColor(Painting::chainColors[chainIdx]);
                curve.mIsHisto = false;
                mGraph->addCurve(curve);
                
                double min = vector_min_value(curve.mDataVector);
                double max = vector_max_value(curve.mDataVector);
                mGraph->setRangeY(floor(min), ceil(max));
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
                mGraph->setRangeX(0, chain.mNumBurnIter + chain.mNumBatchIter * chain.mBatchIndex + chain.mNumRunIter / chain.mThinningInterval);
                mGraph->setRangeY(0, 100);
                
                MHVariable* variable = &(mDate->mTheta);
                if(mCurrentVariable == eTheta) variable = &(mDate->mTheta);
                else if(mCurrentVariable == eSigma) variable = &(mDate->mSigma);
                
                mGraph->addInfo(tr("MCMC") + " : " + ModelUtilities::getDataMethodText(mDate->mMethod));
                mGraph->showInfos(true);
                
                GraphCurve curve;
                curve.mName = QString("accept history chain " + QString::number(chainIdx));
                curve.mData = variable->acceptationForChain(mChains, chainIdx);
                curve.mPen.setColor(Painting::chainColors[chainIdx]);
                curve.mIsHisto = false;
                mGraph->addCurve(curve);
                
                GraphCurve curveTarget;
                curveTarget.mName = "target";
                curveTarget.mIsHorizontalLine = true;
                curveTarget.mHorizontalValue = 44;
                curveTarget.mPen.setStyle(Qt::DashLine);
                curveTarget.mPen.setColor(QColor(180, 10, 20));
                mGraph->addCurve(curveTarget);
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
                
                GraphCurve curve;
                curve.mName = QString("correlation chain " + QString::number(chainIdx));
                curve.mDataVector = variable->correlationForChain(chainIdx);
                curve.mUseVectorData = true;
                curve.mPen.setColor(Painting::chainColors[chainIdx]);
                curve.mIsHisto = false;
                mGraph->addCurve(curve);
                
                mGraph->setRangeX(0, 100);
                mGraph->setRangeY(-1, 1);
                
                double n = variable->runTraceForChain(mChains, chainIdx).size();
                double limit = 1.96f / sqrt(n);
                
                //qDebug() << n << ", " <<limit;
                
                GraphCurve curveLimitLower;
                curveLimitLower.mName = QString("correlation limit lower " + QString::number(chainIdx));
                curveLimitLower.mIsHorizontalLine = true;
                curveLimitLower.mHorizontalValue = -limit;
                curveLimitLower.mPen.setColor(Qt::red);
                curveLimitLower.mPen.setStyle(Qt::DotLine);
                mGraph->addCurve(curveLimitLower);
                
                GraphCurve curveLimitUpper;
                curveLimitUpper.mName = QString("correlation limit upper " + QString::number(chainIdx));
                curveLimitUpper.mIsHorizontalLine = true;
                curveLimitUpper.mHorizontalValue = limit;
                curveLimitUpper.mPen.setColor(Qt::red);
                curveLimitUpper.mPen.setStyle(Qt::DotLine);
                mGraph->addCurve(curveLimitUpper);
            }
            
            /*mGraph->setRangeY(0, 1);
            mGraph->setRangeX(0, 2000);
            
            GraphCurve curve;
            curve.mName = QString("Repartition");
            curve.mData = normalize_map(mDate->mRepartition);
            curve.mPen.setColor(Qt::blue);
            curve.mIsHisto = true;
            mGraph->addCurve(curve);*/
        }
    }
}


