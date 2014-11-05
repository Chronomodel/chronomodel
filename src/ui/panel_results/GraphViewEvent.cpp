#include "GraphViewEvent.h"
#include "GraphView.h"
#include "Event.h"
#include "StdUtilities.h"
#include "QtUtilities.h"
#include "Painting.h"
#include <QtWidgets>



#pragma mark Constructor / Destructor

GraphViewEvent::GraphViewEvent(QWidget *parent):GraphViewResults(parent),
mEvent(0)
{
    //setMainColor(QColor(100, 100, 120));
    setMainColor(QColor(100, 100, 100));
    mGraph->setBackgroundColor(QColor(230, 230, 230));
}

GraphViewEvent::~GraphViewEvent()
{
    
}

void GraphViewEvent::setEvent(Event* event)
{
    if(event)
        mEvent = event;
    update();
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
        p.fillRect(topRect.adjusted(1, 1, -1, 0), backCol);
        
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


void GraphViewEvent::showHisto(bool showAllChains, const QList<bool>& showChainList, bool showHPD, int thresholdHPD)
{
    mGraph->setRangeX(mSettings.mTmin, mSettings.mTmax);
    mGraph->setRangeY(0, 1);
    mGraph->showInfos(false);
    
    if(mEvent)
    {
        QColor color = mEvent->mColor;
        
        mGraph->removeAllCurves();
        mGraph->removeAllZones();
        
        if(showAllChains)
        {
            GraphCurve curve;
            curve.mName = "histo full";
            curve.mPen.setColor(color);
            curve.mData = equal_areas(mEvent->mTheta.fullHisto(), 100.f);
            mGraph->addCurve(curve);
            
            if(showHPD)
            {
                GraphCurve curveHPD;
                curveHPD.mName = "histo HPD full";
                curveHPD.mPen.setColor(color);
                curveHPD.mFillUnder = true;
                curveHPD.mData = equal_areas(mEvent->mTheta.generateFullHPD(thresholdHPD), thresholdHPD);
                mGraph->addCurve(curveHPD);
            }
        }
        for(int i=0; i<showChainList.size(); ++i)
        {
            if(showChainList[i])
            {
                QColor col = Painting::chainColors[i];
                
                GraphCurve curve;
                curve.mName = QString("histo chain " + QString::number(i));
                curve.mPen.setColor(col);
                curve.mData = equal_areas(mEvent->mTheta.histoForChain(i), 100.f);
                mGraph->addCurve(curve);
                
                if(showHPD)
                {
                    GraphCurve curveHPD;
                    curveHPD.mName = QString("histo HPD chain " + QString::number(i));
                    curveHPD.mPen.setColor(color);
                    curveHPD.mFillUnder = true;
                    curveHPD.mData = equal_areas(mEvent->mTheta.generateHPDForChain(i, thresholdHPD), thresholdHPD);
                    mGraph->addCurve(curveHPD);
                }
            }
        }
    }
}

void GraphViewEvent::showTrace(const QList<bool>& showChainList)
{
    mGraph->setRangeX(0, mMCMCSettings.mNumBurnIter + mMCMCSettings.mFinalBatchIndex * mMCMCSettings.mIterPerBatch + mMCMCSettings.mNumRunIter);
    mGraph->showInfos(false);
    
    if(mEvent)
    {
        QColor color = mEvent->mColor;
        
        mGraph->removeAllCurves();
        mGraph->removeAllZones();
        
        // ------
        
        GraphZone zoneBurn;
        zoneBurn.mXStart = 0;
        zoneBurn.mXEnd = mMCMCSettings.mNumBurnIter;
        zoneBurn.mColor = QColor(200, 200, 200);
        zoneBurn.mText = "Burn";
        mGraph->addZone(zoneBurn);
        
        GraphZone zoneAdapt;
        zoneAdapt.mXStart = mMCMCSettings.mNumBurnIter;
        zoneAdapt.mXEnd = mMCMCSettings.mNumBurnIter + mMCMCSettings.mFinalBatchIndex * mMCMCSettings.mIterPerBatch;
        zoneAdapt.mColor = QColor(230, 230, 230);
        zoneAdapt.mText = "Adapt";
        mGraph->addZone(zoneAdapt);
        
        GraphZone zoneAquire;
        zoneAquire.mXStart = mMCMCSettings.mNumBurnIter + mMCMCSettings.mFinalBatchIndex * mMCMCSettings.mIterPerBatch;
        zoneAquire.mXEnd = mMCMCSettings.mNumBurnIter + mMCMCSettings.mFinalBatchIndex * mMCMCSettings.mIterPerBatch + mMCMCSettings.mNumRunIter;
        zoneAquire.mColor = QColor(255, 255, 255);
        zoneAquire.mText = "Aquire";
        mGraph->addZone(zoneAquire);
        
        // ------
        
        float min = 999999;
        float max = -999999;
        
        for(int i=0; i<showChainList.size(); ++i)
        {
            if(showChainList[i])
            {
                QColor col = Painting::chainColors[i];
                
                GraphCurve curve;
                curve.mName = QString("trace chain " + QString::number(i)).toUtf8();
                curve.mUseVectorData = true;
                curve.mDataVector = mEvent->mTheta.traceForChain(i, showChainList.size());
                curve.mPen.setColor(col);
                mGraph->addCurve(curve);
                
                min = qMin(vector_min_value(curve.mDataVector), min);
                max = qMax(vector_max_value(curve.mDataVector), max);
            }
        }
        if(min < max)
            mGraph->setRangeY(min, max);
        else
            mGraph->setRangeY(0, 1);
    }
}

void GraphViewEvent::showAccept(const QList<bool>& showChainList)
{
    mGraph->setRangeX(0, mMCMCSettings.mNumBurnIter + mMCMCSettings.mFinalBatchIndex * mMCMCSettings.mIterPerBatch + mMCMCSettings.mNumRunIter);
    mGraph->setRangeY(0, 100);
    mGraph->clearInfos();
    mGraph->showInfos(true);
    
    QStringList infos;
    
    if(mEvent)
    {
        QColor color = mEvent->mColor;
        
        mGraph->removeAllCurves();
        mGraph->removeAllZones();
        
        // ------
        
        GraphZone zoneBurn;
        zoneBurn.mXStart = 0;
        zoneBurn.mXEnd = mMCMCSettings.mNumBurnIter;
        zoneBurn.mColor = QColor(200, 200, 200);
        zoneBurn.mText = "Burn";
        mGraph->addZone(zoneBurn);
        
        GraphZone zoneAdapt;
        zoneAdapt.mXStart = mMCMCSettings.mNumBurnIter;
        zoneAdapt.mXEnd = mMCMCSettings.mNumBurnIter + mMCMCSettings.mFinalBatchIndex * mMCMCSettings.mIterPerBatch;
        zoneAdapt.mColor = QColor(230, 230, 230);
        zoneAdapt.mText = "Adapt";
        mGraph->addZone(zoneAdapt);
        
        GraphZone zoneAquire;
        zoneAquire.mXStart = mMCMCSettings.mNumBurnIter + mMCMCSettings.mFinalBatchIndex * mMCMCSettings.mIterPerBatch;
        zoneAquire.mXEnd = mMCMCSettings.mNumBurnIter + mMCMCSettings.mFinalBatchIndex * mMCMCSettings.mIterPerBatch + mMCMCSettings.mNumRunIter;
        zoneAquire.mColor = QColor(255, 255, 255);
        zoneAquire.mText = "Aquire";
        mGraph->addZone(zoneAquire);
        
        // ------
        
        GraphCurve curveTarget;
        curveTarget.mName = "target";
        curveTarget.mIsHorizontalLine = true;
        curveTarget.mHorizontalValue = 44;
        curveTarget.mPen.setStyle(Qt::DashLine);
        curveTarget.mPen.setColor(QColor(180, 10, 20));
        mGraph->addCurve(curveTarget);
        
        for(int i=0; i<showChainList.size(); ++i)
        {
            if(showChainList[i])
            {
                QColor col = Painting::chainColors[i];
                
                GraphCurve curve;
                curve.mName = QString("accept history chain " + QString::number(i));
                curve.mDataVector = mEvent->mTheta.acceptationForChain(i, showChainList.size());
                curve.mUseVectorData = true;
                curve.mPen.setColor(col);
                mGraph->addCurve(curve);
                
                if(curve.mDataVector.size() > 0)
                {
                    QString info(tr("Chain") + " " + QString::number(i+1) + " : " + QString::number(curve.mDataVector[curve.mDataVector.size()-1], 'f', 1) + " %");
                    mGraph->addInfo(info);
                }
            }
        }
    }
}
