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
mVariable(eTheta),
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
        
        if(mCurrentResult == eHisto)
        {
            mGraph->setRangeY(0, 0.0001f);
            
            if(mVariable == eTheta)
                mGraph->setRangeX(mSettings.mTmin, mSettings.mTmax);
            else if(mVariable == eSigma || mVariable == eDelta)
                mGraph->setRangeX(0, mSettings.mTmax - mSettings.mTmin);
            
            MHVariable* variable = &(mDate->mTheta);
            if(mVariable == eTheta) variable = &(mDate->mTheta);
            else if(mVariable == eSigma) variable = &(mDate->mSigma);
            else if(mVariable == eDelta) variable = &(mDate->mDelta);
            
            if(mShowCalib && mVariable == eTheta)
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
            mGraph->setRangeX(0, mMCMCSettings.mNumBurnIter + mMCMCSettings.mFinalBatchIndex * mMCMCSettings.mIterPerBatch + mMCMCSettings.mNumRunIter);
            
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
            if(mVariable == eTheta) variable = &(mDate->mTheta);
            else if(mVariable == eSigma) variable = &(mDate->mSigma);
            else if(mVariable == eDelta) variable = &(mDate->mDelta);
            
            float min = 999999;
            float max = -999999;
            
            for(int i=0; i<mShowChainList.size(); ++i)
            {
                if(mShowChainList[i])
                {
                    QColor col = Painting::chainColors[i];
                    
                    GraphCurve curve;
                    curve.mName = QString("trace chain " + QString::number(i));
                    curve.mDataVector = variable->traceForChain(i, mShowChainList.size());
                    curve.mUseVectorData = true;
                    curve.mPen.setColor(Painting::chainColors[i]);
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
        else if(mCurrentResult == eAccept)
        {
            mGraph->setRangeX(0, mMCMCSettings.mNumBurnIter + mMCMCSettings.mFinalBatchIndex * mMCMCSettings.mIterPerBatch + mMCMCSettings.mNumRunIter);
            mGraph->setRangeY(0, 100);
            
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
            if(mVariable == eTheta) variable = &(mDate->mTheta);
            else if(mVariable == eSigma) variable = &(mDate->mSigma);
            else if(mVariable == eDelta) variable = &(mDate->mDelta);
            
            GraphCurve curveTarget;
            curveTarget.mName = "target";
            curveTarget.mIsHorizontalLine = true;
            curveTarget.mHorizontalValue = 44;
            curveTarget.mPen.setStyle(Qt::DashLine);
            curveTarget.mPen.setColor(QColor(180, 10, 20));
            mGraph->addCurve(curveTarget);
            
            for(int i=0; i<mShowChainList.size(); ++i)
            {
                if(mShowChainList[i])
                {
                    QColor col(20 + i*20, 50 + i*20, 80 + i*20);
                    
                    GraphCurve curve;
                    curve.mName = QString("accept history chain " + QString::number(i));
                    curve.mDataVector = variable->acceptationForChain(i, mShowChainList.size());
                    curve.mUseVectorData = true;
                    curve.mPen.setColor(Painting::chainColors[i]);
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
        else if(mCurrentResult == eCorrel)
        {
            
        }
    }
}

void GraphViewDate::setVariableToShow(Variable v)
{
    mVariable = v;
    refresh();
}

void GraphViewDate::showCalib(bool show)
{
    mShowCalib = show;
    refresh();
}

