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
    mMinHeightForButtonsVisible = 80;
    
    setMainColor(QColor(50, 50, 50));
    mGraph->setBackgroundColor(QColor(210, 210, 210));
    //mGraph->setRangeX(mSettings.mTmin, mSettings.mTmax); // it's done in GraphViewResults
    
    mDurationGraph = new GraphView(this);
    mDurationGraph -> setBackgroundColor(QColor(230, 230, 230));
    mDurationGraph -> addInfo(tr("WARNING : this graph scale is NOT the study period!"));
    mDurationGraph -> mLegendX = "";
    mDurationGraph -> setFormatFunctX(0);
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
        setItemTitle(tr("Phase") + " : " + mPhase->mName);
        setItemColor(mPhase->mColor);
    }
    update();
}

void GraphViewPhase::updateLayout()
{
    GraphViewResults::updateLayout();
    
    mShowDuration->setVisible(mButtonVisible);
    
    int leftShift = mForceHideButtons ? 0 : mGraphLeft;
    QRect graphRect(leftShift, mTopShift, this->width() - leftShift, height()-mTopShift);
    
    if(mButtonVisible)
    {
        int butInlineMaxH = 50;
        int bh = (height() - mLineH) / 2;
        bh = qMin(bh, butInlineMaxH);
        mShowDuration->setGeometry(0, mLineH + bh, mGraphLeft, bh);
        
        mDurationGraph->setYAxisMode(GraphView::eMinMax);
        mDurationGraph->setXAxisMode(GraphView::eAllTicks);
        mDurationGraph->setMarginBottom(mGraph->font().pointSizeF() + 10);
    }
    else
    {
        mDurationGraph->setYAxisMode(GraphView::eHidden);
        mDurationGraph->setXAxisMode(GraphView::eHidden);
        mDurationGraph->setMarginBottom(0);
    }
    mDurationGraph->setGeometry(graphRect.adjusted(0, 0, 0, mShowNumResults ? -graphRect.height()/2 : 0));
}

void GraphViewPhase::paintEvent(QPaintEvent* e)
{
    GraphViewResults::paintEvent(e);
}

void GraphViewPhase::generateCurves(TypeGraph typeGraph, Variable variable)
{
    GraphViewResults::generateCurves(typeGraph, variable);
    
    mGraph->removeAllCurves();
    mGraph->removeAllZones();
    mGraph->clearInfos();
    mGraph->resetNothingMessage();
    
    mGraph->autoAdjustYScale(typeGraph == eTrace);
    
    mDurationGraph->removeAllCurves();
    QPen defaultPen;
    defaultPen.setWidthF(1);
    defaultPen.setStyle(Qt::SolidLine);
    
    
    if(mPhase)
    {
        QColor color = mPhase->mColor;
        
        // ------------------------------------------------
        //  first tab : posterior distrib
        //  Possible curves :
        //  - Post Distrib Alpha All Chains
        //  - Post Distrib Beta All Chains
        //  - HPD Alpha All Chains
        //  - HPD Beta All Chains
        //  - Duration
        //  - HPD Duration
        //  - Post Distrib Alpha i
        //  - Post Distrib Beta i
        // ------------------------------------------------
        if(typeGraph == ePostDistrib && variable == eTheta)
        {
            mGraph->mLegendX = DateUtils::getAppSettingsFormat();
            mGraph->setFormatFunctX(DateUtils::convertToAppSettingsFormatStr);
            mGraph->setRangeX(mSettings.mTmin, mSettings.mTmax);
            
            mShowDuration->setVisible(true);
            showDuration(mShowDuration->isChecked());
            
            QString results = ModelUtilities::phaseResultsText(mPhase);
            setNumericalResults(results);
            
            
            GraphCurve curveAlpha = generateDensityCurve(mPhase->mAlpha.fullHisto(),
                                                         "Post Distrib Alpha All Chains",
                                                         color, Qt::DotLine);
            
            GraphCurve curveBeta = generateDensityCurve(mPhase->mBeta.fullHisto(),
                                                         "Post Distrib Beta All Chains",
                                                         color, Qt::DashLine);
            
            
            GraphCurve curveAlphaHPD = generateHPDCurve(mPhase->mAlpha.mHPD,
                                                         "HPD Alpha All Chains",
                                                         color);
            
            GraphCurve curveBetaHPD = generateHPDCurve(mPhase->mBeta.mHPD,
                                                       "HPD Beta All Chains",
                                                       color);
            
            GraphCurve curveDuration = generateDensityCurve(mPhase->mDuration.fullHisto(),
                                                            "Duration",
                                                            color);
            mGraph->addCurve(curveBeta);
            mGraph->addCurve(curveAlpha);
            mGraph->addCurve(curveAlphaHPD);
            mGraph->addCurve(curveBetaHPD);
            mDurationGraph->addCurve(curveDuration);
            
            double max = qMax(map_max_value(curveAlpha.mData), map_max_value(curveBeta.mData));
           
            
            
            if(!curveDuration.mData.isEmpty())
            {
                GraphCurve curveDurationHPD = generateHPDCurve(mPhase->mDuration.mHPD,
                                                               "HPD Duration",
                                                               color);
                mDurationGraph->addCurve(curveDurationHPD);

             }
            
            
            for(int i=0; i<mShowChainList.size(); ++i)
            {
                GraphCurve curveAlpha = generateDensityCurve(mPhase->mAlpha.histoForChain(i),
                                                             "Post Distrib Alpha " + QString::number(i),
                                                             color, Qt::DotLine);
                
                GraphCurve curveBeta = generateDensityCurve(mPhase->mBeta.histoForChain(i),
                                                            "Post Distrib Beta " + QString::number(i),
                                                            color, Qt::DashLine);
                mGraph->addCurve(curveAlpha);
                mGraph->addCurve(curveBeta);
                
                max = qMax(max, map_min_value(curveBeta.mData));
                max = qMax(max, map_max_value(curveAlpha.mData));
                
            }
           
            mGraph->setRangeY(0, max);
        }
        
    
        // ------------------------------------------------
        //  second tab : history plot
        //  - Trace Alpha i
        //  - Q1 Alpha i
        //  - Q2 Alpha i
        //  - Q3 Alpha i
        //  - Trace Beta i
        //  - Q1 Beta i
        //  - Q2 Beta i
        //  - Q3 Beta i
        // ------------------------------------------------
        else if(typeGraph == eTrace && variable == eTheta)
        {
            mGraph->mLegendX = "Iterations";
            mGraph->setFormatFunctX(0);
            
            mShowDuration->setVisible(false);
            mShowDuration->setChecked(false);
            showDuration(false);
            
            generateTraceCurves(mChains, &(mPhase->mAlpha), "Alpha");
            generateTraceCurves(mChains, &(mPhase->mBeta), "Beta");
            
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

void GraphViewPhase::updateCurvesToShow(bool showAllChains, const QList<bool>& showChainList, bool showCredibility, bool showCalib, bool showWiggle)
{
    GraphViewResults::updateCurvesToShow(showAllChains, showChainList, showCredibility, showCalib, showWiggle);
    
    if(mPhase)
    {
        // ------------------------------------------------
        //  first tab : posterior distrib
        //  Possible curves :
        //  - Post Distrib Alpha All Chains
        //  - Post Distrib Beta All Chains
        //  - HPD Alpha All Chains
        //  - HPD Beta All Chains
        //  - Duration
        //  - HPD Duration
        //  - Post Distrib Alpha i
        //  - Post Distrib Beta i
        // ------------------------------------------------
        if(mCurrentTypeGraph == ePostDistrib && mCurrentVariable == eTheta)
        {
            mGraph->setCurveVisible("Post Distrib Alpha All Chains", mShowAllChains);
            mGraph->setCurveVisible("Post Distrib Beta All Chains", mShowAllChains);
            mGraph->setCurveVisible("HPD Alpha All Chains", mShowAllChains);
            mGraph->setCurveVisible("HPD Beta All Chains", mShowAllChains);
            mDurationGraph->setCurveVisible("Duration", mShowAllChains);
            mDurationGraph->setCurveVisible("HPD Duration", mShowAllChains);
            
            for(int i=0; i<mShowChainList.size(); ++i)
            {
                mGraph->setCurveVisible("Post Distrib Alpha " + QString::number(i), mShowChainList[i]);
                mGraph->setCurveVisible("Post Distrib Beta " + QString::number(i), mShowChainList[i]);
            }
            mGraph->adjustYToMaxValue();
            mDurationGraph->adjustYToMaxValue();
        }
        
        // ------------------------------------------------
        //  second tab : history plot
        //  - Alpha Trace i
        //  - Alpha Q1 i
        //  - Alpha Q2 i
        //  - Alpha Q3 i
        //  - Beta Trace i
        //  - Beta Q1 i
        //  - Beta Q2 i
        //  - Beta Q3 i
        // ------------------------------------------------
        else if(mCurrentTypeGraph == eTrace && mCurrentVariable == eTheta)
        {
            for(int i=0; i<mShowChainList.size(); ++i){
                mGraph->setCurveVisible("Alpha Trace " + QString::number(i), mShowChainList[i]);
                mGraph->setCurveVisible("Alpha Q1 " + QString::number(i), mShowChainList[i]);
                mGraph->setCurveVisible("Alpha Q2 " + QString::number(i), mShowChainList[i]);
                mGraph->setCurveVisible("Alpha Q3 " + QString::number(i), mShowChainList[i]);
                
                mGraph->setCurveVisible("Beta Trace " + QString::number(i), mShowChainList[i]);
                mGraph->setCurveVisible("Beta Q1 " + QString::number(i), mShowChainList[i]);
                mGraph->setCurveVisible("Beta Q2 " + QString::number(i), mShowChainList[i]);
                mGraph->setCurveVisible("Beta Q3 " + QString::number(i), mShowChainList[i]);
            }
        }
    }
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
