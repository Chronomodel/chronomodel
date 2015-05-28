#include "GraphViewPhase.h"
#include "GraphView.h"
#include "Phase.h"
#include "Painting.h"
#include "StdUtilities.h"
#include "QtUtilities.h"
#include "ModelUtilities.h"
#include "Button.h"
#include <QtWidgets>



#pragma mark Constructor / Destructor

GraphViewPhase::GraphViewPhase(QWidget *parent):GraphViewResults(parent),
mPhase(0)
{
    setMainColor(QColor(50, 50, 50));
    mGraph->setBackgroundColor(QColor(210, 210, 210));
    //mGraph->setRangeX(mSettings.mTmin, mSettings.mTmax); // it's done in GraphViewResults
    
    mDurationGraph = new GraphView(this);
    mDurationGraph -> setBackgroundColor(QColor(230, 230, 230));
    mDurationGraph -> addInfo(tr("WARNING : this graph scale is NOT the study period!"));
    
    mDurationGraph -> showHorizGrid(false);
    mDurationGraph -> setXAxisMode(GraphView::eAllTicks);
    mDurationGraph -> setYAxisMode(GraphView::eMinMax);
    mDurationGraph -> setBackgroundColor(QColor(210, 210, 210));
    
    mDurationGraph->setMargins(50, 10, 5, 30);
    mDurationGraph->setRangeY(0, 1);
    
    mDurationGraph->setVisible(false);
    
    mShowDuration = new Button(tr("Show Duration"), this);
    mShowDuration->setCheckable(true);
    mShowDuration->setFlatHorizontal();
    connect(mShowDuration, SIGNAL(toggled(bool)), this, SLOT(showDuration(bool)));
}

GraphViewPhase::~GraphViewPhase()
{
    mPhase = 0;
}

void GraphViewPhase::setGraphFont(const QFont& font)
{
    GraphViewResults::setGraphFont(font);
    mDurationGraph->setGraphFont(font);
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
    
    if(mPhase)  {
        this->setItemColor(mPhase->mColor);
        this->setItemTitle(mTitle);
        
    }
    
    
    int h = height();
    int butMinH = 30;
    
    QRect graphRect(mGraphLeft, 0, width() - mGraphLeft, height()-1);
    
    if(h <= mLineH + butMinH)
    {
        mDurationGraph->setYAxisMode(GraphView::eHidden);
    }
    else
    {
        mDurationGraph->setYAxisMode(GraphView::eMinMax);
    }
    
    if(height() >= mMinHeighttoDisplayTitle)
    {
        graphRect.adjust(0, 20, 0, 0);
        mDurationGraph->setXAxisMode(GraphView::eAllTicks);
        mDurationGraph->setMarginBottom(mGraph->font().pointSizeF() + 10);
    }
    else
    {
        mDurationGraph->setXAxisMode(GraphView::eHidden);
        mDurationGraph->setMarginBottom(0);
    }
    
    if(mShowNumResults && height() >= 100)
    {
        mDurationGraph->setGeometry(graphRect.adjusted(0, 0, 0, -graphRect.height()/2));
    }
    else
    {
        mDurationGraph->setGeometry(graphRect);
    }
    
    
    int butInlineMaxH = 50;
    int bh = height() - mLineH;
    bh = qMin(bh, butInlineMaxH);
    
    this->mShowDuration->setVisible(this->GraphViewResults::mButtonVisible);
    mShowDuration->setGeometry(0, mLineH + bh, mGraphLeft, bh);
    
    
    
    
    
}

void GraphViewPhase::refresh()
{
    mGraph->removeAllCurves();
    mGraph->removeAllZones();
    mGraph->clearInfos();
    mGraph->resetNothingMessage();
    
    mGraph->autoAdjustYScale(mCurrentTypeGraph == eTrace);

    mDurationGraph->removeAllCurves();
    QPen defaultPen;
    defaultPen.setWidthF(1);
    defaultPen.setStyle(Qt::SolidLine);

    
    if(mPhase)
    {
        QColor color = mPhase->mColor;

        /* Dessine une densité*/
        if(mCurrentTypeGraph == eHisto && mCurrentVariable == eTheta)
        {
            mShowDuration->setVisible(true);
            /*
             mShowDuration->setChecked(false);
            showDuration(false);
             */
            showDuration(mShowDuration->isChecked());
            
            QString results = ModelUtilities::phaseResultsText(mPhase);
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
                curveAlpha.mName = QString(tr("alpha full"));
                curveAlpha.setPen(defaultPen);
                curveAlpha.mPen.setColor(alphaCol);
                curveAlpha.mPen.setStyle(Qt::DotLine);
                curveAlpha.mIsHisto = false;
                /*if(mPhase->mIsAlphaFixed)
                {
                    curveAlpha.mIsRectFromZero = true;
                    curveAlpha.mData[mSettings.mTmin] = 0.f;
                    curveAlpha.mData[mSettings.mTmax] = 0.f;
                    curveAlpha.mData[floor(mPhase->mAlpha.mX)] = 1.f;
                }
                else
                { */
                    //curveAlpha.mData = equal_areas(mPhase->mAlpha.fullHisto(), 1.f);
                curveAlpha.mData = mPhase->mAlpha.fullHisto();    
                // }
                mGraph->addCurve(curveAlpha);
                
//                double yMax = 1.1f * map_max_value(curveAlpha.mData);
                double yMax = map_max_value(curveAlpha.mData);
                mGraph->setRangeY(0, qMax(mGraph->maximumY(), yMax));
                
                GraphCurve curveBeta;
                curveBeta.mName = QString(tr("beta full"));
                curveBeta.setPen(defaultPen);
                curveBeta.mPen.setColor(betaCol);
                curveBeta.mPen.setStyle(Qt::DashLine);
                curveBeta.mIsHisto = false;
                /*if(mPhase->mIsBetaFixed)
                {
                    curveBeta.mIsRectFromZero = true;
                    curveBeta.mData[mSettings.mTmin] = 0.f;
                    curveBeta.mData[mSettings.mTmax] = 0.f;
                    curveBeta.mData[floor(mPhase->mBeta.mX)] = 1.f;
                }
                else
                { */
                   // curveBeta.mData = equal_areas(mPhase->mBeta.fullHisto(), 1.f);
                curveBeta.mData = mPhase->mBeta.fullHisto();
                    
               // }
                mGraph->addCurve(curveBeta);
                
//                yMax = 1.1f * map_max_value(curveBeta.mData);
                yMax = map_max_value(curveBeta.mData);
                mGraph->setRangeY(0, qMax(mGraph->maximumY(), yMax));
                
                // HPD
                
                GraphCurve curveAlphaHPD;
                curveAlphaHPD.mName = QString(tr("alpha HPD full"));
                curveAlphaHPD.setPen(defaultPen);
                curveAlphaHPD.mPen.setColor(alphaCol);
                curveAlphaHPD.mPen.setStyle(Qt::DotLine);
                curveAlphaHPD.mFillUnder = true;
                //curveAlphaHPD.mBrush.setStyle(Qt::SolidPattern);
                QColor HPDAlphaColor(alphaCol);
                HPDAlphaColor.setAlpha(125);
                curveAlphaHPD.mBrush.setColor(HPDAlphaColor);
                curveAlphaHPD.mIsHisto = false;
                curveAlphaHPD.mIsRectFromZero = true;
                //double realThresh = map_area(mPhase->mAlpha.mHPD) / map_area(mPhase->mAlpha.fullHisto());
                //qDebug()<<"alpha"<<realThresh;
                //curveAlphaHPD.mData = equal_areas(mPhase->mAlpha.mHPD, realThresh);
                curveAlphaHPD.mData = mPhase->mAlpha.mHPD;
                
                mGraph->addCurve(curveAlphaHPD);
                
                GraphCurve curveBetaHPD;
                curveBetaHPD.mName = QString(tr("beta HPD full"));
                curveBetaHPD.setPen(defaultPen);
                curveBetaHPD.mPen.setColor(betaCol);
                curveBetaHPD.mPen.setStyle(Qt::DashLine);
                curveBetaHPD.mFillUnder = true;
                
                //curveBetaHPD.mBrush.setStyle(Qt::SolidPattern);
                QColor HPDBetaColor(betaCol);
                HPDBetaColor.setAlpha(125);
                curveBetaHPD.mBrush.setColor(HPDBetaColor);
                
                curveBetaHPD.mIsHisto = false;
                curveBetaHPD.mIsRectFromZero = true;
                //realThresh = map_area(mPhase->mBeta.mHPD) / map_area(mPhase->mBeta.fullHisto());
                //qDebug()<<"beta"<<realThresh;
                //curveBetaHPD.mData = equal_areas(mPhase->mBeta.mHPD, realThresh);
                curveBetaHPD.mData = mPhase->mBeta.mHPD;
                mGraph->addCurve(curveBetaHPD);
                
                // Duration
                
                GraphCurve curveDur;
                curveDur.mName = mTitle+" : "+QString(tr("duration"));
                curveDur.setPen(defaultPen);
                curveDur.mPen.setColor(betaCol);
                curveDur.mIsHisto = false;
                //curveDur.mData = equal_areas(mPhase->mDuration.fullHisto(), 1.f);
                curveDur.mData = mPhase->mDuration.fullHisto();
                
                if(!curveDur.mData.isEmpty())
                {
                    mDurationGraph->addCurve(curveDur);
                    
                    yMax = 1.1f * map_max_value(curveDur.mData);
                    //mDurationGraph->setRangeY(0, qMax(mGraph->maximumY(), 0.000001));
                    //mDurationGraph->setRangeX(0, qMax(curveDur.mData.lastKey(), 0.01)); //map_max_value
                    
                    mDurationGraph->setRangeX(0, curveDur.mData.lastKey());
                    mDurationGraph->setCurrentX(0, mDurationGraph->maximumX());
                    /* add curve of duration with HPD */
                    GraphCurve curveDurHPD;
                    curveDurHPD.mName = QString(tr("duration HPD"));
                    curveDurHPD.setPen(defaultPen);
                    curveDurHPD.mPen.setColor(color);
                    curveDurHPD.mFillUnder = true;
                    QColor HPDColor(color);
                    HPDColor.setAlpha(100);
                    curveDurHPD.mBrush.setStyle(Qt::SolidPattern);
                    curveDurHPD.mBrush.setColor(HPDColor);
                    curveDurHPD.mIsHisto = false;
                    curveDurHPD.mIsRectFromZero = true;
                    //double realThresh = map_area(mPhase->mDuration.mHPD) / map_area(mPhase->mDuration.fullHisto());
                    
                    //curveDurHPD.mData = equal_areas(mPhase->mDuration.mHPD, realThresh);
                    curveDurHPD.mData = mPhase->mDuration.mHPD;

                    double max = map_max_value(curveDurHPD.mData);

                    mDurationGraph->setRangeY(0, max);
                    mDurationGraph->addCurve(curveDurHPD);
                }
                /* Draw alpha and beta without smoothing*/
                if(mShowRawResults)
                {
                    GraphCurve curveRawAlpha;
                    curveRawAlpha.mName = mTitle+" : "+QString(tr("raw"));
                    curveRawAlpha.mPen.setColor(Qt::red);
                    curveRawAlpha.mPen.setStyle(Qt::DotLine);
                    //curveRawAlpha.mData = equal_areas(mPhase->mAlpha.fullRawHisto(), 1.f);
                    curveRawAlpha.mData = mPhase->mAlpha.fullRawHisto();
                    curveRawAlpha.mIsHisto = true;
                    mGraph->addCurve(curveRawAlpha);
                    
                    yMax = 1.1f * map_max_value(curveRawAlpha.mData);
                    mGraph->setRangeY(0, qMax(mGraph->maximumY(), yMax));
                    
                    GraphCurve curveRawBeta;
                    curveRawBeta.mName = mTitle+" : "+QString(tr("raw beta"));
                    curveRawBeta.mPen.setColor(Qt::red);
                    curveRawBeta.mPen.setStyle(Qt::DashLine);
                    //curveRawBeta.mData = equal_areas(mPhase->mBeta.fullRawHisto(), 1.f);
                    curveRawBeta.mData = mPhase->mBeta.fullRawHisto();
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
                    curveAlphaChain.mName = mTitle+" : "+QString(tr("alpha chain ") + QString::number(i));
                    curveAlphaChain.mPen.setColor(col);
                    curveAlphaChain.mPen.setStyle(Qt::DotLine);
                    curveAlphaChain.mIsHisto = false;
                    //curveAlphaChain.mData = equal_areas(mPhase->mAlpha.histoForChain(i), 1.f);
                    curveAlphaChain.mData = mPhase->mAlpha.histoForChain(i);
                    mGraph->addCurve(curveAlphaChain);
                    
                    double yMax = 1.1f * map_max_value(curveAlphaChain.mData);
                    mGraph->setRangeY(0, qMax(mGraph->maximumY(), yMax));
                    
                    GraphCurve curveBetaChain;
                    curveBetaChain.mName = QString(tr("beta chain ") + QString::number(i));
                    curveBetaChain.mPen.setColor(col);
                    curveBetaChain.mPen.setStyle(Qt::DashLine);
                    curveBetaChain.mIsHisto = false;
                    //curveBetaChain.mData = equal_areas(mPhase->mBeta.histoForChain(i), 1.f);
                    curveBetaChain.mData = mPhase->mBeta.histoForChain(i);
                    mGraph->addCurve(curveBetaChain);
                    
                    yMax = 1.1f * map_max_value(curveBetaChain.mData);
                    mGraph->setRangeY(0, qMax(mGraph->maximumY(), yMax));
                }
            }
        }

        /* Dessine une trace*/
        else if(mCurrentTypeGraph == eTrace && mCurrentVariable == eTheta)
        {
            mShowDuration->setVisible(false);
            mShowDuration->setChecked(false);
            showDuration(false);
            
            int chainIdx = -1;
            for(int i=0; i<mShowChainList.size(); ++i)
                if(mShowChainList[i])
                    chainIdx = i;
            
            if(chainIdx != -1)
            {
                Chain& chain = mChains[chainIdx];
                //mGraph->setCurrentX(0, chain.mNumBurnIter + chain.mNumBatchIter * chain.mBatchIndex + chain.mNumRunIter / chain.mThinningInterval);
                mGraph->setRangeX(0, chain.mNumBurnIter + chain.mNumBatchIter * chain.mBatchIndex + chain.mNumRunIter / chain.mThinningInterval);
                
                QColor col = Painting::chainColors[chainIdx];
                
                GraphCurve curveAlpha;
                curveAlpha.mUseVectorData = true;
                curveAlpha.mName = mTitle+" : "+QString(tr("trace chain ") + QString::number(chainIdx));
                curveAlpha.mDataVector = mPhase->mAlpha.fullTraceForChain(mChains, chainIdx);
                curveAlpha.mPen.setColor(col);

                curveAlpha.mIsHisto = false;
                mGraph->addCurve(curveAlpha);
                
                GraphCurve curveBeta;
                curveBeta.mUseVectorData = true;
                curveBeta.mName = QString(tr("beta trace chain ") + QString::number(chainIdx));
                curveBeta.mDataVector = mPhase->mBeta.fullTraceForChain(mChains, chainIdx);
                curveBeta.mPen.setColor(col);

                curveBeta.mIsHisto = false;
                mGraph->addCurve(curveBeta);
                
                double min = qMin(vector_min_value(curveBeta.mDataVector), vector_min_value(curveAlpha.mDataVector));
                double max = qMax(vector_max_value(curveBeta.mDataVector), vector_max_value(curveAlpha.mDataVector));
                
                mGraph->setRangeY(min, max);
            }
        }
    }
}

void GraphViewPhase::updateLayout()
{
   // GraphViewResults::updateLayout();
   // GraphViewResults::repaint();
 /*   int h = height();
    int butMinH = 30;
    
    QRect graphRect(mGraphLeft, 0, width() - mGraphLeft, height()-1);

    if(h <= mLineH + butMinH)
    {
        mDurationGraph->setYAxisMode(GraphView::eHidden);
    }
    else
    {
        mDurationGraph->setYAxisMode(GraphView::eMinMax);
    }

    if(height() >= mMinHeighttoDisplayTitle)
    {
        graphRect.adjust(0, 20, 0, 0);
        mDurationGraph->setXAxisMode(GraphView::eAllTicks);
        mDurationGraph->setMarginBottom(mGraph->font().pointSizeF() + 10);
    }
    else
    {
        mDurationGraph->setXAxisMode(GraphView::eHidden);
        mDurationGraph->setMarginBottom(0);
    }
    
    if(mShowNumResults && height() >= 100)
    {
        mDurationGraph->setGeometry(graphRect.adjusted(0, 0, 0, -graphRect.height()/2));
    }
    else
    {
        mDurationGraph->setGeometry(graphRect);
    }
    
    
    int butInlineMaxH = 50;
    int bh = height() - mLineH;
    bh = qMin(bh, butInlineMaxH);
    
    this->mShowDuration->setVisible(this->GraphViewResults::mButtonVisible);
    mShowDuration->setGeometry(0, mLineH + bh, mGraphLeft, bh);
  */  
    
}

void GraphViewPhase::showDuration(bool show)
{
    mDurationGraph->setVisible(show);
    mGraph->setVisible(!show);
    mShowDuration->raise();
}
/* double GraphViewPhase::getMaxDuration()
{

    if (mPhase->mDuration.) {
    double max = mPhase->mDuration.mHisto.lastKey();
    
    return max;
    }
    else return 0.0;
}*/
