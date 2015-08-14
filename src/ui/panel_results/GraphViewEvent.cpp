#include "GraphViewEvent.h"
#include "GraphView.h"
#include "Event.h"
#include "EventKnown.h"
#include "StdUtilities.h"
#include "QtUtilities.h"
#include "ModelUtilities.h"
#include "Painting.h"
#include "MainWindow.h"
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
    mEvent = 0;
}

void GraphViewEvent::setEvent(Event* event)
{
    if(event)
    {
        mEvent = event;
        QString eventTitle = ( (mEvent->mType == Event::eDefault) ? tr("Event") : tr("Bound") ) ;
        this->setItemTitle(eventTitle + " : " + mEvent->mName);
        setItemColor(mEvent->mColor);
    }
    update();
}

void GraphViewEvent::paintEvent(QPaintEvent* e)
{
    GraphViewResults::paintEvent(e);
}

void GraphViewEvent::generateCurves(TypeGraph typeGraph, Variable variable)
{
    GraphViewResults::generateCurves(typeGraph, variable);
    
    // ------------------------------------------------
    //  Reset the graph object settings
    // ------------------------------------------------
    mGraph->removeAllCurves();
    mGraph->removeAllZones();
    mGraph->clearInfos();
    mGraph->resetNothingMessage();
    
    mGraph->autoAdjustYScale(mCurrentTypeGraph == eTrace);
    
    QPen defaultPen;
    defaultPen.setWidthF(1);
    defaultPen.setStyle(Qt::SolidLine);
    
    if(mEvent)
    {
        QColor color = mEvent->mColor;
        
        bool isFixedBound = false;
        bool isUnifBound = false;
        EventKnown* bound = 0;
        if(mEvent->type() == Event::eKnown)
        {
            bound = dynamic_cast<EventKnown*>(mEvent);
            if(bound)
            {
                if(bound->knownType() == EventKnown::eFixed)
                    isFixedBound = true;
                else if(bound->knownType() == EventKnown::eUniform)
                    isUnifBound = true;
            }
        }
        
        QString results = ModelUtilities::eventResultsText(mEvent, false);
        setNumericalResults(results);
        
        // ------------------------------------------------
        //  First tab : Posterior distrib
        // ------------------------------------------------
        if(typeGraph == ePostDistrib)
        {
            mGraph->mLegendX = DateUtils::getAppSettingsFormat();
            mGraph->setFormatFunctX(DateUtils::convertToAppSettingsFormatStr);
            mGraph->setBackgroundColor(QColor(230, 230, 230));
            
            mTitle = ((mEvent->type()==Event::eKnown) ? tr("Bound ") : tr("Event")) + " : " + mEvent->mName;
            
            // ------------------------------------------------
            //  Possible curves :
            //  - Post Distrib All Chains
            //  - HPD All Chains
            //  - Credibility All Chains
            //  - Post Distrib Chain i
            // ------------------------------------------------
            if(variable == eTheta)
            {
                mGraph->setRangeX(mSettings.mTmin, mSettings.mTmax);
                
                // ------------------------------------
                //  Post Distrib All Chains
                // ------------------------------------
                GraphCurve curvePostDistrib;
                curvePostDistrib.mName = "Post Distrib All Chains";
                curvePostDistrib.mPen.setColor(color);
                
                if(isFixedBound)
                {
                    curvePostDistrib.mIsHisto = false;
                    curvePostDistrib.mIsRectFromZero = true;
                    for(int t=mSettings.mTmin; t< mSettings.mTmax; ++t)
                        curvePostDistrib.mData[t] = 0.f;
                    curvePostDistrib.mData[floor(bound->fixedValue())] = 1.f;
                }
                else if(isUnifBound)
                {
                    curvePostDistrib.mIsHisto = true;
                    curvePostDistrib.mIsRectFromZero = true;
                    curvePostDistrib.mData = bound->mValues;
                }
                else
                {
                    curvePostDistrib = generateDensityCurve(mEvent->mTheta.fullHisto(),
                                                                       "Post Distrib All Chains",
                                                                       color);
                }
                mGraph->addCurve(curvePostDistrib);
                
                // ------------------------------------
                //  HPD All Chains
                // ------------------------------------
                if(!isFixedBound)
                {
                    // HPD All Chains
                    GraphCurve curveHPD = generateHPDCurve(mEvent->mTheta.mHPD,
                                                           "HPD All Chains",
                                                           color);
                    mGraph->addCurve(curveHPD);
                }
                
                // ------------------------------------
                //  Post Distrib Chain i
                // ------------------------------------
                for(int i=0; i<mShowChainList.size(); ++i)
                {
                    GraphCurve curvePostDistribChain = generateDensityCurve(mEvent->mTheta.histoForChain(i),
                                                                            "Post Distrib Chain " + QString::number(i),
                                                                            Painting::chainColors[i],
                                                                            Qt::SolidLine,
                                                                            Qt::NoBrush);
                    mGraph->addCurve(curvePostDistribChain);
                }
                
                // ------------------------------------
                //  Theta Credibility
                // ------------------------------------
                GraphCurve curveCred = generateCredibilityCurve(mEvent->mTheta.mCredibility,
                                                                "Credibility All Chains",
                                                                color);
                mGraph->addCurve(curveCred);
            }
            
            
            // ------------------------------------------------
            //  Events don't have std dev BUT we can visualize
            //  an overlay of all dates std dev instead.
            //  Possible curves, FOR ALL DATES :
            //  - Sigma Date i All Chains
            //  - Sigma Date i Chain j
            // ------------------------------------------------
            else if(variable == eSigma)
            {
                mGraph->mLegendX = "";
                mGraph->setFormatFunctX(0);
                //mGraph->setRangeX(0,mSettings.mTmin + mSettings.mTmax);
                if (mEvent->type()==Event::eKnown) {
                    mTitle = tr("Bound ") + " : " + mEvent->mName;
                }
                else
                    mTitle = tr("Std") + " : " + mEvent->mName;
                
                mGraph->setBackgroundColor(QColor(Qt::white));
                mGraph->autoAdjustYScale(true);

                mGraph->setRangeX(0, mSettings.mTmax - mSettings.mTmin);

                
                for(int i=0; i<mEvent->mDates.size(); ++i)
                {
                    Date& date = mEvent->mDates[i];
                    
                    GraphCurve curve = generateDensityCurve(date.mSigma.fullHisto(),
                                                            "Sigma Date " + QString::number(i) + " All Chains",
                                                            color);
                    
                    mGraph->addCurve(curve);
                    
                    for(int j=0; j<mShowChainList.size(); ++j)
                    {
                        GraphCurve curveChain = generateDensityCurve(date.mSigma.histoForChain(j),
                                                                     "Sigma Date " + QString::number(i) + " Chain " + QString::number(j),
                                                                     Painting::chainColors[j]);
                        mGraph->addCurve(curveChain);
                    }
                }
            }
        }
        // ------------------------------------------------
        //  second tab : History plots
        //  - Trace i
        //  - Q1 i
        //  - Q2 i
        //  - Q3 i
        // ------------------------------------------------
        else if(typeGraph == eTrace && variable == eTheta)
        {
            mGraph->mLegendX = "Iterations";
            mGraph->setFormatFunctX(0);
            
            generateTraceCurves(mChains, &(mEvent->mTheta));
        }
        // ------------------------------------------------
        //  third tab : Acception rate
        //  - Accept i
        //  - Accept Target
        // ------------------------------------------------
        else if(typeGraph == eAccept && variable == eTheta && mEvent->mMethod == Event::eMHAdaptGauss)
        {
            mGraph->mLegendX = "Iterations";
            mGraph->setFormatFunctX(0);
            mGraph->setRangeY(0, 100);
            
            generateHorizontalLine(44, "Accept Target", QColor(180, 10, 20), Qt::DashLine);
            generateAcceptCurves(mChains, &(mEvent->mTheta));
        }
        // ------------------------------------------------
        //  fourth tab : Autocorrelation
        //  - Correl i
        //  - Correl Limit Lower i
        //  - Correl Limit Upper i
        // ------------------------------------------------
        else if(typeGraph == eCorrel && variable == eTheta && !isFixedBound)
        {
            mGraph->mLegendX = "";
            mGraph->setFormatFunctX(0);
            mGraph->setRangeX(0, 100);
            mGraph->setRangeY(-1, 1);
            
            generateCorrelCurves(mChains, &(mEvent->mTheta));
        }
    }
}

void GraphViewEvent::updateCurvesToShow(bool showAllChains, const QList<bool>& showChainList, bool showCredibility, bool showCalib, bool showWiggle)
{
    GraphViewResults::updateCurvesToShow(showAllChains, showChainList, showCredibility, showCalib, showWiggle);
    
    if(mEvent)
    {
        bool isFixedBound = false;
        bool isUnifBound = false;
        EventKnown* bound = 0;
        if(mEvent->type() == Event::eKnown)
        {
            bound = dynamic_cast<EventKnown*>(mEvent);
            if(bound)
            {
                if(bound->knownType() == EventKnown::eFixed)
                    isFixedBound = true;
                else if(bound->knownType() == EventKnown::eUniform)
                    isUnifBound = true;
            }
        }
        
        // ------------------------------------------------
        //  first tab : Posterior distrib
        // ------------------------------------------------
        if(mCurrentTypeGraph == ePostDistrib)
        {
            // ------------------------------------------------
            //  Possible curves :
            //  - Post Distrib All Chains
            //  - HPD All Chains
            //  - Credibility All Chains
            //  - Post Distrib Chain i
            // ------------------------------------------------
            if(mCurrentVariable == eTheta)
            {
                mGraph->setCurveVisible("Post Distrib All Chains", mShowAllChains);
                mGraph->setCurveVisible("HPD All Chains", mShowAllChains);
                mGraph->setCurveVisible("Credibility All Chains", mShowCredibility && mShowAllChains);
                
                for(int i=0; i<mShowChainList.size(); ++i){
                    mGraph->setCurveVisible("Post Distrib Chain " + QString::number(i), mShowChainList[i]);
                }
            }
            // ------------------------------------------------
            //  Events don't have std dev BUT we can visualize
            //  an overlay of all dates std dev instead.
            //  Possible curves, FOR ALL DATES :
            //  - Sigma Date i All Chains
            //  - Sigma Date i Chain j
            // ------------------------------------------------
            else if(mCurrentVariable == eSigma)
            {
                for(int i=0; i<mEvent->mDates.size(); ++i)
                {
                    mGraph->setCurveVisible("Sigma Date " + QString::number(i) + " All Chains", mShowAllChains);
                    
                    for(int j=0; j<mShowChainList.size(); ++j)
                    {
                        mGraph->setCurveVisible("Sigma Date " + QString::number(i) + " Chain " + QString::number(j), mShowChainList[j]);
                    }
                }
            }
            mGraph->adjustYToMaxValue();
        }
        // ------------------------------------------------
        //  Second tab : History plots
        //  Possible curves :
        //  - Trace i
        //  - Q1 i
        //  - Q2 i
        //  - Q3 i
        // ------------------------------------------------
        else if(mCurrentTypeGraph == eTrace && mCurrentVariable == eTheta)
        {
            // We visualize only one chain (radio button)
            for(int i=0; i<mShowChainList.size(); ++i){
                mGraph->setCurveVisible("Trace " + QString::number(i), mShowChainList[i]);
                mGraph->setCurveVisible("Q1 " + QString::number(i), mShowChainList[i]);
                mGraph->setCurveVisible("Q2 " + QString::number(i), mShowChainList[i]);
                mGraph->setCurveVisible("Q3 " + QString::number(i), mShowChainList[i]);
            }
        }
        
        // ------------------------------------------------
        //  Third tab : Acception rate
        //  Possible curves :
        //  - Accept i
        //  - Accept Target
        // ------------------------------------------------
        else if(mCurrentTypeGraph == eAccept && mCurrentVariable == eTheta && mEvent->mMethod == Event::eMHAdaptGauss)
        {
            mGraph->setCurveVisible("Accept Target", true);
            for(int i=0; i<mShowChainList.size(); ++i){
                mGraph->setCurveVisible("Accept " + QString::number(i), mShowChainList[i]);
            }
        }
        // ------------------------------------------------
        //  fourth tab : Autocorrelation
        //  Possible curves :
        //  - Correl i
        //  - Correl Limit Lower i
        //  - Correl Limit Upper i
        // ------------------------------------------------
        else if(mCurrentTypeGraph == eCorrel && mCurrentVariable == eTheta && !isFixedBound)
        {
            for(int i=0; i<mShowChainList.size(); ++i){
                mGraph->setCurveVisible("Correl " + QString::number(i), mShowChainList[i]);
                mGraph->setCurveVisible("Correl Limit Lower " + QString::number(i), mShowChainList[i]);
                mGraph->setCurveVisible("Correl Limit Upper " + QString::number(i), mShowChainList[i]);
            }
        }
    }
}
