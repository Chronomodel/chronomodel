#include "GraphViewDate.h"
#include "GraphView.h"
#include "Date.h"
#include "Event.h"
#include "Painting.h"
#include "StdUtilities.h"
#include "QtUtilities.h"
#include "ModelUtilities.h"
#include <QtWidgets>
#include "PluginAbstract.h"
#include "GraphViewRefAbstract.h"



#pragma mark Constructor / Destructor

GraphViewDate::GraphViewDate(QWidget *parent):GraphViewResults(parent),
mDate(0),
mColor(Qt::blue)
{
    //setMainColor(QColor(150, 150, 150));
    setMainColor(QColor(155, 155, 155));
    mGraph->setBackgroundColor(Qt::white);
    if(mCurrentVariable == eTheta)
        mGraph->setRangeX(mSettings.mTmin, mSettings.mTmax);
    else if(mCurrentVariable == eSigma)
        mGraph->setRangeX(0, mSettings.mTmax - mSettings.mTmin);
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
        mTitle = QString(tr("Data") + " : " + mDate->mName);
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
    if(mDate)
    {
        this->setItemColor(mColor);
        this->setItemTitle(tr("Data") + " : " + mDate->mName);
    }
  GraphViewResults::paintEvent(e);
}

void GraphViewDate::refresh()
{
    mGraph->removeAllCurves();
    mGraph->removeAllZones();
    mGraph->clearInfos();
    mGraph->resetNothingMessage();
    mGraph->setMaximumY(0);
    mGraph->autoAdjustYScale(mCurrentResult == eTrace);
    
    if(mDate)
    {
        QColor color = mColor;
        
        QString results = ModelUtilities::dateResultsText(mDate);
        setNumericalResults(results);
        
        if(mCurrentResult == eHisto)
        {
            mGraph->setRangeY(0, 0.0001f);
            
          /*  if(mCurrentVariable == eTheta)
                mGraph->setRangeX(mSettings.mTmin, mSettings.mTmax);
            else if(mCurrentVariable == eSigma)
                mGraph->setRangeX(0, mSettings.mTmax - mSettings.mTmin);
          */
            
          /*  MHVariable* variable = &(mDate->mTheta);
            if(mCurrentVariable == eTheta) variable = &(mDate->mTheta);
            else if(mCurrentVariable == eSigma) variable = &(mDate->mSigma);
          */
            if(mShowCalib && mCurrentVariable == eTheta)
            {
                GraphCurve curve;
                curve.mName = "calibration";
                //if(mDate->mCalibration.isEmpty()) {
                //if(mDate->mCalibration.isEmpty()) {
                //    mDate->calibrate(mSettings);
               //     qDebug()<<"mDate->calibrate(mSettings);";
               // }
                //curve.mData = equal_areas(mDate->getCalibMap(), 1.f);
                
                curve.mData = mDate->getCalibMap();
                //curve.mData = mDate->mCalibration;
                QString namePlugin = mDate->mPlugin->getName();
                QColor dataColor   = QColor(Qt::black);//mDate->mPlugin->getColor();//QColor(120, 120, 120); ///since 28/04/2015
                QIcon dataIcon     = mDate->mPlugin->getIcon();
                curve.mName = "calibration : "+namePlugin;
                
                curve.mPen.setColor(dataColor);
                //curve.mPen.setColor(Qt::black);
                curve.mPen.setStyle(Qt::SolidLine);
               
                curve.mBrush.setStyle(Qt::VerPattern);//Qt::LinearGradientPattern);//Qt::HorPattern);// Qt::Dense6Pattern);VerPattern
                QColor brushColor(dataColor);
                //brushColor.setAlpha(70); //50
                curve.mBrush.setColor(brushColor);
                
                curve.mFillUnder = false;//!mShowPosterior; //since 28/04/2015
                
                curve.mIsHisto = false;
                curve.mIsRectFromZero = true; // for typo. calibs., invisible for others!
                mGraph->addCurve(curve);
                
                qDebug()<<"mName"<<mDate->mName;
              
                double yMax = 1.1f * map_max_value(curve.mData);
                //qDebug()<<"yMax"<<yMax<<" mSetting.mTmin"<<mSettings.mTmin<<" mSettings.mTmin"<<mSettings.mTmax<<" mStep"<<mSettings.mStep;;
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
            
            MHVariable* variable = &(mDate->mTheta);
            if(mCurrentVariable == eTheta) variable = &(mDate->mTheta);
            else if(mCurrentVariable == eSigma) variable = &(mDate->mSigma);

            if(mShowAllChains)
            {
                
                if(mShowRawResults)
                {
                    GraphCurve curveRaw;
                    curveRaw.mName = "raw histo full";
                    curveRaw.mPen.setColor(Qt::red);
                    curveRaw.mData = equal_areas(variable->fullRawHisto(), 1.f);
                    curveRaw.mIsHisto = true;
                    // mGraph->mZones[0].mText="curveRaw";
                    mGraph->addCurve(curveRaw);
                    
                    double yMax2 = 1.1f * map_max_value(curveRaw.mData);
                    mGraph->setRangeY(0, qMax(mGraph->maximumY(), yMax2));
                }

                
                if (mShowPosterior) {
                   QColor HPDColor(color);
                    HPDColor.setAlpha(100);//HPDColor.setAlpha(50); // since 28/04/2015
                    GraphCurve curve;
                    curve.mName = "histo full";
                    curve.mData = equal_areas(variable->fullHisto(), 1.f);
                    curve.mPen.setColor(HPDColor);
                    curve.mIsHisto = false;
                    curve.mFillUnder = false;
                    mGraph->addCurve(curve);
                    double yMax = 1.1f * map_max_value(curve.mData);
                    mGraph->setRangeY(0, qMax(mGraph->maximumY(), yMax));
                    
                    
                    /*
                     GraphCurve curve;
                     curve.mName = "histo full";
                     curve.mPen.setColor(color);
                     curve.mData = equal_areas(mEvent->mTheta.fullHisto(), 1.f);
                     curve.mIsHisto = false;
                     mGraph->addCurve(curve);
                     */
                    
                    if(mCurrentVariable != GraphViewResults::eSigma)
                    {
                        GraphCurve curveHPD;
                        curveHPD.mName = "histo HPD full";
                        curveHPD.mData = equal_areas(variable->mHPD, mThresholdHPD/100.f);
                        curveHPD.mPen.setColor(color);
                        curveHPD.mFillUnder = true;
                        curveHPD.mBrush.setStyle(Qt::SolidPattern);
                        //QColor HPDColor(color);
                        //HPDColor.setAlpha(50);
                        curveHPD.mBrush.setColor(HPDColor);
                        curveHPD.mIsHisto = false;
                        curveHPD.mIsRectFromZero = true;
                        mGraph->addCurve(curveHPD);
                    }
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
                
                const Quartiles& quartiles = variable->mResults.quartiles;
                
                GraphCurve curveQ1;
                curveQ1.mIsHorizontalLine = true;
                curveQ1.mHorizontalValue = quartiles.Q1;
                curveQ1.mName = QString("Q1");
                curveQ1.mPen.setColor(Qt::green);
                mGraph->addCurve(curveQ1);
                
                GraphCurve curveQ2;
                curveQ2.mIsHorizontalLine = true;
                curveQ2.mHorizontalValue = quartiles.Q2;
                curveQ2.mName = QString("Q2");
                curveQ2.mPen.setColor(Qt::red);
                mGraph->addCurve(curveQ2);
                
                GraphCurve curveQ3;
                curveQ3.mIsHorizontalLine = true;
                curveQ3.mHorizontalValue = quartiles.Q3;
                curveQ3.mName = QString("Q3");
                curveQ3.mPen.setColor(Qt::green);
                mGraph->addCurve(curveQ3);
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
                
                if(mCurrentVariable == eTheta)
                    mGraph->addInfo(ModelUtilities::getDataMethodText(mDate->mMethod));
                else if(mCurrentVariable == eSigma)
                    mGraph->addInfo(ModelUtilities::getDataMethodText(Date::eMHSymGaussAdapt));
                mGraph->showInfos(true);
                
                GraphCurve curve;
                curve.mName = QString("accept history chain " + QString::number(chainIdx));
                curve.mDataVector = variable->acceptationForChain(mChains, chainIdx);
                curve.mPen.setColor(Painting::chainColors[chainIdx]);
                curve.mUseVectorData = true;
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
            
            if(chainIdx != -1 && chainIdx < mChains.size())
            {
                if(true)//mChains[chainIdx].mThinningInterval == 1)
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
                else
                {
                    // -----------------------------------------------
                    //  Important : auto-correlation must be calculated on  ALL TRACE VALUES !!
                    //  Otherwise, it is false. => The thinning must be 1!
                    //  TODO : find a solution to calculate auto-correlation on all trace points
                    //  without having to store all trace points (with thinning > 1)
                    // -----------------------------------------------
                    
                    mGraph->setNothingMessage(tr("The thinning interval must be 1 to display this curve."));
                }
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


