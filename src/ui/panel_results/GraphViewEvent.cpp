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


// Constructor / Destructor

GraphViewEvent::GraphViewEvent(QWidget *parent):GraphViewResults(parent),
mEvent(nullptr)
{
    setMainColor(QColor(100, 100, 100));
    mGraph->setBackgroundColor(QColor(230, 230, 230));
}

GraphViewEvent::~GraphViewEvent()
{
    mEvent = nullptr;
}

void GraphViewEvent::setEvent(Event* event)
{
    Q_ASSERT(event);

    mEvent = event;
    QString eventTitle = ( (mEvent->mType == Event::eDefault) ? tr("Event") : tr("Bound") ) ;
    this->setItemTitle(eventTitle + " : " + mEvent->mName);
    setItemColor(mEvent->mColor);

    update();
}

void GraphViewEvent::paintEvent(QPaintEvent* e)
{
    GraphViewResults::paintEvent(e);
}

void GraphViewEvent::generateCurves(TypeGraph typeGraph, Variable variable)
{
    Q_ASSERT(mEvent);
    qDebug()<<"GraphViewEvent::generateCurves()";
    GraphViewResults::generateCurves(typeGraph, variable);
    
    /* ------------------------------------------------
     *  Reset the graph object settings
     * ------------------------------------------------
     */
    mGraph->removeAllCurves();
    mGraph->reserveCurves(6);

    mGraph->removeAllZones();
    mGraph->clearInfos();
    mGraph->resetNothingMessage();

    mGraph->setOverArrow(GraphView::eNone);

    QPen defaultPen;
    defaultPen.setWidthF(1);
    defaultPen.setStyle(Qt::SolidLine);
    

    QColor color = mEvent->mColor;
    QString resultsText = ModelUtilities::eventResultsText(mEvent, false);
    QString resultsHTML = ModelUtilities::eventResultsHTML(mEvent, false);
    setNumericalResults(resultsHTML, resultsText);

    bool isFixedBound = false;
    EventKnown* bound = nullptr;
    if (mEvent->type() == Event::eKnown) {
        bound = dynamic_cast<EventKnown*>(mEvent);
        if (bound)
                isFixedBound = true;

    }

    // ------------------------------------------------
    //  First tab : Posterior distrib
    // ------------------------------------------------
    if (typeGraph == ePostDistrib) {
        mGraph->mLegendX = DateUtils::getAppSettingsFormatStr();
        mGraph->setFormatFunctX(stringWithAppSettings);
        mGraph->setFormatFunctY(nullptr);
        mGraph->setBackgroundColor(QColor(230, 230, 230));



        mTitle = ((mEvent->type()==Event::eKnown) ? tr("Bound ") : tr("Event")) + " : " + mEvent->mName;

        /* ------------------------------------------------
         *  Possible curves :
         *  - Post Distrib All Chains
         *  - HPD All Chains
         *  - Credibility All Chains
         *  - Post Distrib Chain i
         * ------------------------------------------------
         */
        if (variable == eTheta) {
            mGraph->setOverArrow(GraphView::eBothOverflow);
            if (isFixedBound) {
                GraphCurve curveLineBound;
                curveLineBound.mName = "Post Distrib All Chains";
                curveLineBound.mPen.setColor(color);
                curveLineBound.mIsVerticalLine = true;
                curveLineBound.mVerticalValue = bound->fixedValue();
                mGraph->addCurve(curveLineBound);

            }


            // ------------------------------------
            //  HPD All Chains
            // ------------------------------------
            if (!isFixedBound) {
                /* ------------------------------------
                 *  Post Distrib All Chains
                 * ------------------------------------
                 */
                GraphCurve curvePostDistrib;
                curvePostDistrib.mName = "Post Distrib All Chains";
                curvePostDistrib.mPen.setColor(color);

                curvePostDistrib = generateDensityCurve(mEvent->mTheta.fullHisto(),
                                                                       "Post Distrib All Chains",
                                                                       color);
                mGraph->addCurve(curvePostDistrib);

                // HPD All Chains
                GraphCurve curveHPD = generateHPDCurve(mEvent->mTheta.mHPD,
                                                       "HPD All Chains",
                                                       color);
                mGraph->addCurve(curveHPD);

                /* ------------------------------------
                 *  Post Distrib Chain i
                 * ------------------------------------
                 */
                if (!mEvent->mTheta.mChainsHistos.isEmpty())
                    for (int i=0; i<mChains.size(); ++i) {
                        GraphCurve curvePostDistribChain = generateDensityCurve(mEvent->mTheta.histoForChain(i),
                                                                                "Post Distrib Chain " + QString::number(i),
                                                                                Painting::chainColors.at(i),
                                                                                Qt::SolidLine,
                                                                                Qt::NoBrush);
                        mGraph->addCurve(curvePostDistribChain);
                    }

                /* ------------------------------------
                 *  Theta Credibility
                 * ------------------------------------
                 */
                GraphCurve curveCred = generateSectionCurve(mEvent->mTheta.mCredibility,
                                                                "Credibility All Chains",
                                                                color);
                mGraph->addCurve(curveCred);
            }
            // ------------------------------------------------------------
            //  Add zones outside study period
            // ------------------------------------------------------------

            GraphZone zoneMin;
            zoneMin.mXStart = -INFINITY;
            zoneMin.mXEnd = mSettings.getTminFormated();
            zoneMin.mColor = QColor(217, 163, 69);
            zoneMin.mColor.setAlpha(35);
            zoneMin.mText = tr("Outside study period");
            mGraph->addZone(zoneMin);

            GraphZone zoneMax;
            zoneMax.mXStart = mSettings.getTmaxFormated();
            zoneMax.mXEnd = INFINITY;
            zoneMax.mColor = QColor(217, 163, 69);
            zoneMax.mColor.setAlpha(35);
            zoneMax.mText = tr("Outside study period");
            mGraph->addZone(zoneMax);
        }


        /* ------------------------------------------------
         *  Events don't have std dev BUT we can visualize
         *  an overlay of all dates std dev instead.
         *  Possible curves, FOR ALL DATES :
         *  - Sigma Date i All Chains
         *  - Sigma Date i Chain j
         * ------------------------------------------------
         */
        else if (variable == eSigma) {
            mGraph->setOverArrow(GraphView::eNone);
            mGraph->mLegendX = "";
            mGraph->setFormatFunctX(stringWithAppSettings);
            mGraph->setFormatFunctY(stringWithAppSettings);

            if (mEvent->type()==Event::eKnown)
                mTitle = tr("Bound") + " : " + mEvent->mName;
            else
                mTitle = tr("Std Compilation") + " : " + mEvent->mName;

            mGraph->setBackgroundColor(QColor(230, 230, 230));

            int i(0);
            for (auto&& date : mEvent->mDates) {
                GraphCurve curve = generateDensityCurve(date.mSigma.fullHisto(),
                                                        "Sigma Date " + QString::number(i) + " All Chains",
                                                        color);

                mGraph->addCurve(curve);
                if (!date.mSigma.mChainsHistos.isEmpty())
                    for (int j=0; j<mChains.size(); ++j) {
                        GraphCurve curveChain = generateDensityCurve(date.mSigma.histoForChain(j),
                                                                     "Sigma Date " + QString::number(i) + " Chain " + QString::number(j),
                                                                     Painting::chainColors.at(j));
                        mGraph->addCurve(curveChain);
                    }
                ++i;
            }
        }

    }
    /* -------------------second tab : History plots-----------------------------
     *  - Trace i
     *  - Q1 i
     *  - Q2 i
     *  - Q3 i
     * ------------------------------------------------  */
    else if (typeGraph == eTrace && variable == eTheta) {
        mGraph->mLegendX = "Iterations";
        mGraph->setFormatFunctX(nullptr);
        mGraph->setFormatFunctY(stringWithAppSettings);
        mTitle = ((mEvent->type()==Event::eKnown) ? tr("Bound ") : tr("Event")) + " : " + mEvent->mName;

        generateTraceCurves(mChains, &(mEvent->mTheta));

    }
    /* ---------------------third tab : Acceptance rate---------------------------
     *  - Accept i
     *  - Accept Target
     * ------------------------------------------------  */
    else if (typeGraph == eAccept && variable == eTheta
                && (mEvent->mMethod == Event::eMHAdaptGauss || mEvent->mMethod == Event::eFixe)) {
        mGraph->mLegendX = "Iterations";
        mGraph->setFormatFunctX(nullptr);
        mGraph->setFormatFunctY(stringWithAppSettings);
        mTitle = ((mEvent->type()==Event::eKnown) ? tr("Bound ") : tr("Event")) + " : " + mEvent->mName;

        generateHorizontalLine(44, "Accept Target", QColor(180, 10, 20), Qt::DashLine);
        generateAcceptCurves(mChains, &(mEvent->mTheta));
        mGraph->repaint();
    }

    /* ------------------------------------------------
     *  fourth tab : Autocorrelation
     *  - Correl i
     *  - Correl Limit Lower i
     *  - Correl Limit Upper i
     * ------------------------------------------------
     */
    else if ((typeGraph == eCorrel) && (variable == eTheta) && (!isFixedBound)) {
        mGraph->mLegendX = "";
        mGraph->setFormatFunctX(nullptr);
        mGraph->setFormatFunctY(stringWithAppSettings);
        mTitle = ((mEvent->type()==Event::eKnown) ? tr("Bound ") : tr("Event")) + " : " + mEvent->mName;

        generateCorrelCurves(mChains, &(mEvent->mTheta));
    }
    else {
        mTitle = ((mEvent->type()==Event::eKnown) ? tr("Bound") : tr("Event")) + " : " + mEvent->mName;
        mGraph->resetNothingMessage();
    }

}

void GraphViewEvent::updateCurvesToShow(bool showAllChains, const QList<bool>& showChainList, bool showCredibility, bool showCalib, bool showWiggle)
{
    Q_ASSERT(mEvent);
    GraphViewResults::updateCurvesToShow(showAllChains, showChainList, showCredibility, showCalib, showWiggle);
    
    bool isFixedBound = false;

    EventKnown* bound = nullptr;
    if (mEvent->type() == Event::eKnown) {
        bound = dynamic_cast<EventKnown*>(mEvent);
        isFixedBound = true;

    }

    /* --------------------first tab : Posterior distrib----------------------------
     * ------------------------------------------------*/
    if (mCurrentTypeGraph == ePostDistrib) {
        mGraph->setTipYLab("");
        /* ------------------------------------------------
         *  Possible curves :
         *  - Post Distrib All Chains
         *  - HPD All Chains
         *  - Credibility All Chains
         *  - Post Distrib Chain i
         * ------------------------------------------------
         */
        if (mCurrentVariable == eTheta) {
            mGraph->setCurveVisible("Post Distrib All Chains", mShowAllChains);
            mGraph->setCurveVisible("HPD All Chains", mShowAllChains);
            mGraph->setCurveVisible("Credibility All Chains", mShowCredibility && mShowAllChains);

            for (int i=0; i<mShowChainList.size(); ++i)
                mGraph->setCurveVisible("Post Distrib Chain " + QString::number(i), mShowChainList.at(i));

            mGraph->setTipXLab("t");
            mGraph->setYAxisMode(GraphView::eHidden);

        }
        /* ------------------------------------------------
         *  Events don't have std dev BUT we can visualize
         *  an overlay of all dates std dev instead.
         *  Possible curves, FOR ALL DATES :
         *  - Sigma Date i All Chains
         *  - Sigma Date i Chain j
         * ------------------------------------------------
         */
        else if (mCurrentVariable == eSigma) {
            for (int i=0; i<mEvent->mDates.size(); ++i) {
                mGraph->setCurveVisible("Sigma Date " + QString::number(i) + " All Chains", mShowAllChains);

                for (int j=0; j<mShowChainList.size(); ++j)
                    mGraph->setCurveVisible("Sigma Date " + QString::number(i) + " Chain " + QString::number(j), mShowChainList.at(j));

            }
            mGraph->setTipXLab("duration");
            mGraph->setYAxisMode(GraphView::eHidden);

        }
        mGraph->autoAdjustYScale(true);
    }
    /* -------------------- Second tab : History plots----------------------------
     *  Possible curves :
     *  - Trace i
     *  - Q1 i
     *  - Q2 i
     *  - Q3 i
     * ------------------------------------------------  */
    else if (mCurrentTypeGraph == eTrace && mCurrentVariable == eTheta) {
        // We visualize only one chain (radio button)
        for (int i=0; i<mShowChainList.size(); ++i) {
            mGraph->setCurveVisible("Trace " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Q1 " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Q2 " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Q3 " + QString::number(i), mShowChainList.at(i));
        }

        mGraph->setTipXLab("iteration");
        mGraph->setTipYLab("t");
        mGraph->setYAxisMode(GraphView::eMinMax);
        mGraph->autoAdjustYScale(true);
    }

    /* ----------------------Third tab : Acceptance rate--------------------------
     *  Possible curves :
     *  - Accept i
     *  - Accept Target
     * ------------------------------------------------  */
    else if ((mCurrentTypeGraph == eAccept) && (mCurrentVariable == eTheta) && ((mEvent->mMethod == Event::eMHAdaptGauss) || (mEvent->mMethod == Event::eFixe))) {
        mGraph->setCurveVisible("Accept Target", true);
        for (int i=0; i<mShowChainList.size(); ++i)
            mGraph->setCurveVisible("Accept " + QString::number(i), mShowChainList.at(i));

        mGraph->setTipXLab("iteration");
        mGraph->setTipYLab("rate");
        mGraph->setYAxisMode(GraphView::eMinMax);
        mGraph->autoAdjustYScale(false); // do  repaintGraph()
        mGraph->setRangeY(0, 100);
    }
    /* ----------------------fourth tab : Autocorrelation--------------------------
     *  Possible curves :
     *  - Correl i
     *  - Correl Limit Lower i
     *  - Correl Limit Upper i
     * ------------------------------------------------   */
    else if (mCurrentTypeGraph == eCorrel && mCurrentVariable == eTheta && !isFixedBound) {
        for (int i=0; i<mShowChainList.size(); ++i) {
            mGraph->setCurveVisible("Correl " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Correl Limit Lower " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Correl Limit Upper " + QString::number(i), mShowChainList.at(i));
        }
        mGraph->setTipXLab("h");
        mGraph->setTipYLab("value");
        mGraph->setYAxisMode(GraphView::eMinMax);
        mGraph->autoAdjustYScale(false); // do  repaintGraph()
        mGraph->setRangeY(-1, 1);
    }

    update();
}

