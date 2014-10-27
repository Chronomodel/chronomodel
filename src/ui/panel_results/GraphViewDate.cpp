#include "GraphViewDate.h"
#include "GraphView.h"
#include "Date.h"
#include "Event.h"
#include "StdUtilities.h"
#include "QtUtilities.h"
#include <QtWidgets>



#pragma mark Constructor / Destructor

GraphViewDate::GraphViewDate(QWidget *parent):GraphViewResults(parent),
mDate(0)
{
    setMainColor(QColor(100, 120, 100));
}

GraphViewDate::~GraphViewDate()
{
    
}

void GraphViewDate::setDate(Date* date)
{
    if(date)
        mDate = date;
    update();
}

void GraphViewDate::paintEvent(QPaintEvent* e)
{
    GraphViewResults::paintEvent(e);
    
    QPainter p(this);
    
    if(mDate)
    {
        QColor color = Qt::blue; //mDate->mEvent->mColor;
        bool isDark = colorIsDark(color);
        
        QRectF r(mMargin, mMargin, mLineH, mLineH);
        p.setBrush(color);
        p.drawRect(r);
        p.setPen(isDark ? Qt::white : Qt::black);
        p.drawText(r, Qt::AlignCenter, "D");
        
        r = QRectF(2*mMargin + mLineH,
                   mMargin,
                   mGraphLeft - 3*mMargin - mLineH,
                   mLineH);
        
        p.setPen(Qt::black);
        p.drawText(r, Qt::AlignLeft | Qt::AlignVCenter, mDate->mName);
    }
}



void GraphViewDate::showHisto(bool showTheta, bool showSigma, bool showDelta, bool showCalib, bool showAllChains, const QList<bool>& showChainList, bool showHPD, int thresholdHPD)
{
    mGraph->showInfos(false);
    mGraph->setRangeY(0, 1);
    
    if(showTheta)
        mGraph->setRangeX(mSettings.mTmin, mSettings.mTmax);
    else if(showSigma || showDelta)
        mGraph->setRangeX(0, mSettings.mTmax - mSettings.mTmin);
    
    if(mDate)
    {
        QColor color = Qt::blue; //mDate->mEvent->mColor;
        
        mGraph->removeAllCurves();
        mGraph->removeAllZones();
        
        MHVariable* variable = &(mDate->mTheta);
        if(showTheta) variable = &(mDate->mTheta);
        else if(showSigma) variable = &(mDate->mSigma);
        else if(showDelta) variable = &(mDate->mDelta);
        
        if(showCalib && showTheta)
        {
            GraphCurve curve;
            curve.mName = "calibration";
            //curve.mData = normalize_map(mDate->mCalibration);
            curve.mData = equal_areas(mDate->mCalibration, 100.f);
            curve.mPen.setColor(QColor(100, 100, 100));
            curve.mFillUnder = true;
            mGraph->addCurve(curve);
            
            mGraph->setRangeY(0, qMax(mGraph->maximumY(), map_max_value(curve.mData)));
        }
        
        if(showAllChains)
        {
            GraphCurve curve;
            curve.mName = "histo full";
            curve.mData = equal_areas(variable->fullHisto(), 100.f);
            curve.mPen.setColor(color);
            mGraph->addCurve(curve);
            
            mGraph->setRangeY(0, qMax(mGraph->maximumY(), map_max_value(curve.mData)));
            
            if(showHPD)
            {
                GraphCurve curveHPD;
                curveHPD.mName = "histo HPD full";
                curveHPD.mData = equal_areas(variable->generateFullHPD(thresholdHPD), thresholdHPD);
                curveHPD.mPen.setColor(color);
                curveHPD.mFillUnder = true;
                mGraph->addCurve(curveHPD);
            }
        }
        for(int i=0; i<showChainList.size(); ++i)
        {
            if(showChainList[i])
            {
                QColor col = mChainColors[i];
                
                GraphCurve curve;
                curve.mName = QString("histo chain " + QString::number(i));
                curve.mData = equal_areas(variable->histoForChain(i), 100.f);
                curve.mPen.setColor(col);
                mGraph->addCurve(curve);
                
                mGraph->setRangeY(0, qMax(mGraph->maximumY(), map_max_value(curve.mData)));
                
                if(showHPD)
                {
                    GraphCurve curveHPD;
                    curveHPD.mName = QString("histo HPD chain " + QString::number(i));
                    curveHPD.mData = equal_areas(variable->generateHPDForChain(i, thresholdHPD), thresholdHPD);
                    curveHPD.mPen.setColor(col);
                    curveHPD.mFillUnder = true;
                    mGraph->addCurve(curveHPD);
                }
            }
        }
    }
}

void GraphViewDate::showTrace(bool showTheta, bool showSigma, bool showDelta, const QList<bool>& showChainList)
{
    mGraph->setRangeX(0, mMCMCSettings.mNumBurnIter + mMCMCSettings.mFinalBatchIndex * mMCMCSettings.mIterPerBatch + mMCMCSettings.mNumRunIter);
    mGraph->showInfos(false);
    
    if(mDate)
    {
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
        
        MHVariable* variable = &(mDate->mTheta);
        if(showTheta) variable = &(mDate->mTheta);
        else if(showSigma) variable = &(mDate->mSigma);
        else if(showDelta) variable = &(mDate->mDelta);
        
        float min = 999999;
        float max = -999999;
        
        for(int i=0; i<showChainList.size(); ++i)
        {
            if(showChainList[i])
            {
                QColor col = mChainColors[i];
                
                GraphCurve curve;
                curve.mName = QString("trace chain " + QString::number(i));
                curve.mDataVector = variable->traceForChain(i, showChainList.size());
                curve.mUseVectorData = true;
                curve.mPen.setColor(mChainColors[i]);
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

void GraphViewDate::showAccept(bool showTheta, bool showSigma, bool showDelta, const QList<bool>& showChainList)
{
    mGraph->setRangeX(0, mMCMCSettings.mNumBurnIter + mMCMCSettings.mFinalBatchIndex * mMCMCSettings.mIterPerBatch + mMCMCSettings.mNumRunIter);
    mGraph->setRangeY(0, 100);
    mGraph->clearInfos();
    mGraph->showInfos(true);
    
    if(mDate)
    {
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
        
        MHVariable* variable = &(mDate->mTheta);
        if(showTheta) variable = &(mDate->mTheta);
        else if(showSigma) variable = &(mDate->mSigma);
        else if(showDelta) variable = &(mDate->mDelta);
        
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
                QColor col(20 + i*20, 50 + i*20, 80 + i*20);
                
                GraphCurve curve;
                curve.mName = QString("accept history chain " + QString::number(i));
                curve.mDataVector = variable->acceptationForChain(i, showChainList.size());
                curve.mUseVectorData = true;
                curve.mPen.setColor(mChainColors[i]);
                mGraph->addCurve(curve);
                
                if(curve.mDataVector.size() > 0)
                {
                    QString info(tr("Chain") + " " + QString::number(i+1) + " : " +
                                 QString::number(curve.mDataVector[curve.mDataVector.size()-1], 'f', 1) + " %");
                    mGraph->addInfo(info);
                }
            }
        }
    }
}