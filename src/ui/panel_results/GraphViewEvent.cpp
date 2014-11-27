#include "GraphViewEvent.h"
#include "GraphView.h"
#include "Event.h"
#include "StdUtilities.h"
#include "QtUtilities.h"
#include "Painting.h"
#include "MainWindow.h"
#include <QtWidgets>



#pragma mark Constructor / Destructor

GraphViewEvent::GraphViewEvent(QWidget *parent):GraphViewResults(parent),
mEvent(0)
{
    //setMainColor(QColor(100, 100, 120));
    setMainColor(QColor(100, 100, 100));
    mGraph->setBackgroundColor(QColor(240, 240, 240));
}

GraphViewEvent::~GraphViewEvent()
{
    mEvent = 0;
}

void GraphViewEvent::setEvent(Event* event)
{
    if(event)
        mEvent = event;
    update();
}

void GraphViewEvent::refresh()
{
    mGraph->removeAllCurves();
    mGraph->removeAllZones();
    mGraph->clearInfos();
    setNumericalResults("");
    
    if(mEvent)
    {
        QColor color = mEvent->mColor;
        
        if(mCurrentResult == eHisto)
        {
            if(mCurrentVariable == eTheta)
            {
                setNumericalResults(mEvent->mTheta.resultsText());
                
                mGraph->setRangeX(mSettings.mTmin, mSettings.mTmax);
                mGraph->setRangeY(0, 0.00001f);
                
                if(mShowAllChains)
                {
                    GraphCurve curve;
                    curve.mName = "histo full";
                    curve.mPen.setColor(color);
                    curve.mData = equal_areas(mEvent->mTheta.fullHisto(), 100.f);
                    curve.mIsHisto = false;
                    mGraph->addCurve(curve);
                    
                    float yMax = 1.1f * map_max_value(curve.mData);
                    mGraph->setRangeY(0, qMax(mGraph->maximumY(), yMax));
                    
                    if(mShowHPD)
                    {
                        GraphCurve curveHPD;
                        curveHPD.mName = "histo HPD full";
                        curveHPD.mPen.setColor(color);
                        curveHPD.mFillUnder = true;
                        curveHPD.mIsHisto = false;
                        curveHPD.mData = equal_areas(mEvent->mTheta.mHPD, mThresholdHPD);
                        mGraph->addCurve(curveHPD);
                        
                        GraphCurve curveCred;
                        curveCred.mName = "credibility full";
                        curveCred.mSections.append(mEvent->mTheta.mCredibility);
                        curveCred.mHorizontalValue = mGraph->maximumY();
                        curveCred.mPen.setColor(color);
                        curveCred.mPen.setWidth(5);
                        curveCred.mIsHorizontalSections = true;
                        mGraph->addCurve(curveCred);
                    }
                }
                for(int i=0; i<mShowChainList.size(); ++i)
                {
                    if(mShowChainList[i])
                    {
                        QColor col = Painting::chainColors[i];
                        
                        GraphCurve curve;
                        curve.mName = QString("histo chain " + QString::number(i));
                        curve.mPen.setColor(col);
                        curve.mIsHisto = false;
                        curve.mData = equal_areas(mEvent->mTheta.histoForChain(i), 100.f);
                        mGraph->addCurve(curve);
                        
                        float yMax = 1.1f * map_max_value(curve.mData);
                        mGraph->setRangeY(0, qMax(mGraph->maximumY(), yMax));
                        
                        /*if(mShowHPD)
                        {
                            GraphCurve curveHPD;
                            curveHPD.mName = QString("histo HPD chain " + QString::number(i));
                            curveHPD.mPen.setColor(color);
                            curveHPD.mFillUnder = true;
                            curveHPD.mData = equal_areas(mEvent->mTheta.generateHPDForChain(i, mThresholdHPD), mThresholdHPD);
                            mGraph->addCurve(curveHPD);
                        }*/
                    }
                }
            }
            else if(mCurrentVariable == eSigma)
            {
                // On est en train de regarder les variances des data
                // On affiche donc ici la superposition des variances (et pas le résultat de theta f)
                
                mGraph->setRangeX(0, mSettings.mTmax - mSettings.mTmin);
                
                for(int i=0; i<mEvent->mDates.size(); ++i)
                {
                    Date& date = mEvent->mDates[i];
                    if(mShowAllChains)
                    {
                        GraphCurve curve;
                        curve.mName = "histo full date " + QString::number(i);
                        curve.mPen.setColor(color);
                        curve.mIsHisto = false;
                        curve.mData = equal_areas(date.mSigma.fullHisto(), 100.f);
                        mGraph->addCurve(curve);
                        
                        float yMax = 1.1f * map_max_value(curve.mData);
                        mGraph->setRangeY(0, qMax(mGraph->maximumY(), yMax));
                        
                        /*if(mShowHPD)
                        {
                            GraphCurve curveHPD;
                            curveHPD.mName = "histo HPD full";
                            curveHPD.mPen.setColor(color);
                            curveHPD.mFillUnder = true;
                            curveHPD.mData = equal_areas(date.mSigma.generateFullHPD(mThresholdHPD), mThresholdHPD);
                            mGraph->addCurve(curveHPD);
                        }*/
                    }
                    for(int j=0; j<mShowChainList.size(); ++j)
                    {
                        if(mShowChainList[j])
                        {
                            QColor col = Painting::chainColors[j];
                            
                            GraphCurve curve;
                            curve.mName = QString("histo sigma data " + QString::number(i) + " for chain" + QString::number(j));
                            curve.mPen.setColor(col);
                            curve.mIsHisto = false;
                            curve.mData = equal_areas(date.mSigma.histoForChain(j), 100.f);
                            mGraph->addCurve(curve);
                            
                            float yMax = 1.1f * map_max_value(curve.mData);
                            mGraph->setRangeY(0, qMax(mGraph->maximumY(), yMax));
                            
                            /*if(mShowHPD)
                            {
                                GraphCurve curveHPD;
                                curveHPD.mName = QString("hpd sigma data " + QString::number(i) + " for chain" + QString::number(j));
                                curveHPD.mPen.setColor(color);
                                curveHPD.mFillUnder = true;
                                curveHPD.mData = equal_areas(date.mSigma.generateHPDForChain(j, mThresholdHPD), mThresholdHPD);
                                mGraph->addCurve(curveHPD);
                            }*/
                        }
                    }
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
                
                GraphCurve curve;
                curve.mName = QString("trace chain " + QString::number(chainIdx)).toUtf8();
                curve.mData = mEvent->mTheta.fullTraceForChain(mChains, chainIdx);
                curve.mPen.setColor(Painting::chainColors[chainIdx]);
                curve.mIsHisto = false;
                mGraph->addCurve(curve);
                
                float min = map_min_value(curve.mData);
                float max = map_max_value(curve.mData);
                mGraph->setRangeY(floorf(min), ceilf(max));
            }
        }
        else if(mCurrentResult == eAccept && mCurrentVariable == eTheta && mEvent->mMethod == Event::eMHAdaptGauss)
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
                
                GraphCurve curve;
                curve.mName = QString("accept history chain " + QString::number(chainIdx));
                curve.mData = mEvent->mTheta.acceptationForChain(mChains, chainIdx);
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
        else if(mCurrentResult == eCorrel && mCurrentVariable == eTheta)
        {
            int chainIdx = -1;
            for(int i=0; i<mShowChainList.size(); ++i)
                if(mShowChainList[i])
                    chainIdx = i;
            
            if(chainIdx != -1)
            {
                GraphCurve curve;
                curve.mName = QString("correlation chain " + QString::number(chainIdx));
                curve.mDataVector = mEvent->mTheta.correlationForChain(chainIdx);
                curve.mUseVectorData = true;
                curve.mPen.setColor(Painting::chainColors[chainIdx]);
                curve.mIsHisto = false;
                mGraph->addCurve(curve);
                
                mGraph->setRangeX(0, 100);
                mGraph->setRangeY(-1, 1);
            }
        }
    }
}

void GraphViewEvent::paintEvent(QPaintEvent* e)
{
    GraphViewResults::paintEvent(e);
    
    QPainter p(this);
    
    if(mEvent)
    {
        QColor backCol = mEvent->mColor;
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
        QString type = (mEvent->mType == Event::eDefault) ? tr("Event") : tr("Bound");
        p.drawText(topRect.adjusted(mMargin, 0, -mMargin, 0),
                   Qt::AlignVCenter | Qt::AlignLeft,
                   type + " : " + mEvent->mName);
    }
}
