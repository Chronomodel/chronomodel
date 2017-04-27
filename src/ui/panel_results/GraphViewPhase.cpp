#include "GraphViewPhase.h"
#include "GraphView.h"
#include "Phase.h"
#include "Painting.h"
#include "StdUtilities.h"
#include "QtUtilities.h"
#include "DateUtils.h"
#include "ModelUtilities.h"
#include "MainWindow.h"
#include "Button.h"
#include <QtWidgets>

//pragma mark Constructor / Destructor

GraphViewPhase::GraphViewPhase(QWidget *parent):GraphViewResults(parent),
mPhase(nullptr)
{
    setMainColor(QColor(50, 50, 50));
    mGraph->setBackgroundColor(QColor(210, 210, 210));

}

GraphViewPhase::~GraphViewPhase()
{
    mPhase = nullptr;
}

void GraphViewPhase::setGraphFont(const QFont& font)
{
    GraphViewResults::setFont(font);
    updateLayout();
}


void GraphViewPhase::setPhase(Phase* phase)
{
    Q_ASSERT(phase);

    mPhase = phase;
    setItemTitle(tr("Phase") + " : " + mPhase->mName);

    setItemColor(mPhase->mColor);

}

void GraphViewPhase::updateLayout()
{
        GraphViewResults::updateLayout();
}

void GraphViewPhase::paintEvent(QPaintEvent* e)
{
    GraphViewResults::paintEvent(e);
}

void GraphViewPhase::resizeEvent(QResizeEvent* )
{
    updateLayout();
}

void GraphViewPhase::generateCurves(TypeGraph typeGraph, Variable variable)
{
    //qDebug()<<"GraphViewPhase::generateCurves()";
    Q_ASSERT(mPhase);
    GraphViewResults::generateCurves(typeGraph, variable);
    
    mGraph->removeAllCurves();
    mGraph->reserveCurves(9);

    mGraph->removeAllZones();
    mGraph->clearInfos();
    mGraph->resetNothingMessage();
    
    QPen defaultPen;
    defaultPen.setWidthF(1.);
    defaultPen.setStyle(Qt::SolidLine);
    
    QColor color = mPhase->mColor;

    QString resultsText = ModelUtilities::phaseResultsText(mPhase);
    QString resultsHTML = ModelUtilities::phaseResultsHTML(mPhase);
    setNumericalResults(resultsHTML, resultsText);

    mGraph->setOverArrow(GraphView::eNone);
    /* -------------first tab : posterior distrib-----------------------------------
     *  Possible curves :
     *  - Post Distrib Alpha All Chains
     *  - Post Distrib Beta All Chains
     *  - HPD Alpha All Chains
     *  - HPD Beta All Chains
     *  - Duration
     *  - HPD Duration
     *  - Post Distrib Alpha i
     *  - Post Distrib Beta i
     *  - Time Range
     * ------------------------------------------------  */
    if ((typeGraph == ePostDistrib) && (variable == eTheta)) {
        mGraph->mLegendX = DateUtils::getAppSettingsFormatStr();
        mGraph->mLegendY = "";
        mGraph->setFormatFunctX(stringWithAppSettings);
        mGraph->setFormatFunctY(nullptr);
        mTitle = tr("Phase") + " : " + mPhase->mName;

        GraphCurve curveAlpha = generateDensityCurve(mPhase->mAlpha.fullHisto(),
                                                     "Post Distrib Alpha All Chains",
                                                     color, Qt::DotLine);
        QColor colorBeta = mPhase->mColor.darker(170);

        GraphCurve curveBeta = generateDensityCurve(mPhase->mBeta.fullHisto(),
                                                     "Post Distrib Beta All Chains",
                                                     colorBeta, Qt::DashLine);

        color.setAlpha(255); // set mBrush to fill
        GraphCurve curveAlphaHPD = generateHPDCurve(mPhase->mAlpha.mHPD,
                                                     "HPD Alpha All Chains",
                                                     color);

        GraphCurve curveBetaHPD = generateHPDCurve(mPhase->mBeta.mHPD,
                                                   "HPD Beta All Chains",
                                                   colorBeta);

        mGraph->addCurve(curveAlpha);
        mGraph->addCurve(curveBeta);


        mGraph->addCurve(curveAlphaHPD);
        mGraph->addCurve(curveBetaHPD);

        mGraph->setOverArrow(GraphView::eBothOverflow);

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

        if (!mPhase->mAlpha.mChainsHistos.isEmpty())
            for (int i=0; i<mChains.size(); ++i) {
                GraphCurve curveAlpha = generateDensityCurve(mPhase->mAlpha.histoForChain(i),
                                                             "Post Distrib Alpha " + QString::number(i),
                                                             color, Qt::DotLine);

                GraphCurve curveBeta = generateDensityCurve(mPhase->mBeta.histoForChain(i),
                                                            "Post Distrib Beta " + QString::number(i),
                                                            colorBeta, Qt::DashLine);
                mGraph->addCurve(curveAlpha);
                mGraph->addCurve(curveBeta);
            }
    }


    else if ((typeGraph == ePostDistrib) && (variable == eDuration)) {
        mGraph->mLegendX = DateUtils::getAppSettingsFormatStr();
        mGraph->mLegendY = "";
        mGraph->setFormatFunctX(stringWithAppSettings);
        mGraph->setFormatFunctY(nullptr);

        mTitle = tr("Phase Duration") + " : " + mPhase->mName;
        GraphCurve curveDuration;

        if (mPhase->mDuration.fullHisto().size()>1) {
            curveDuration = generateDensityCurve(mPhase->mDuration.fullHisto(),
                                                            "Post Distrib Duration All Chains",
                                                            color);

            mGraph->setRangeX(0., ceil(curveDuration.mData.lastKey()));
            color.setAlpha(255);
            GraphCurve curveDurationHPD = generateHPDCurve(mPhase->mDuration.mHPD,
                                                           "HPD Duration All Chains",
                                                           color);
            mGraph->setCanControlOpacity(true);
            mGraph->addCurve(curveDurationHPD);
            mGraph->setFormatFunctX(stringWithAppSettings);
            mGraph->setFormatFunctY(nullptr);

            mGraph->addCurve(curveDuration);

        } else
            mGraph->resetNothingMessage();


        if (!mPhase->mDuration.mChainsHistos.isEmpty())
            for (int i=0; i<mChains.size(); ++i) {
                GraphCurve curveDuration = generateDensityCurve(mPhase->mDuration.histoForChain(i),
                                                             "Post Distrib Duration " + QString::number(i),
                                                             color, Qt::DotLine);

                mGraph->addCurve(curveDuration);
            }
     }

    /* -----------------second tab : history plot-------------------------------
     *  - Trace Alpha i
     *  - Q1 Alpha i
     *  - Q2 Alpha i
     *  - Q3 Alpha i
     *  - Trace Beta i
     *  - Q1 Beta i
     *  - Q2 Beta i
     *  - Q3 Beta i
     * ------------------------------------------------ */
    else if (typeGraph == eTrace && variable == eTheta) {
        mGraph->mLegendX = "Iterations";
        mGraph->setFormatFunctX(nullptr);
        mGraph->setFormatFunctY(stringWithAppSettings);

        generateTraceCurves(mChains, &(mPhase->mAlpha), "Alpha");
        generateTraceCurves(mChains, &(mPhase->mBeta), "Beta");
        mGraph->autoAdjustYScale(true);
    }
    else if (typeGraph == eTrace && variable == eDuration) {
        mGraph->mLegendX = "Iterations";
        mGraph->setFormatFunctX(nullptr);
        mGraph->setFormatFunctY(stringWithAppSettings);

        generateTraceCurves(mChains, &(mPhase->mDuration), "Duration");
        mGraph->autoAdjustYScale(true);
    }
    /* ------------------------------------------------
     *  third tab : Acception rate
     *  fourth tab : Autocorrelation
     * ------------------------------------------------ */
   // else if ((typeGraph == eAccept) || (typeGraph == eCorrel) ) {
    else
       mGraph->resetNothingMessage();
  //  }


}

void GraphViewPhase::updateCurvesToShow(bool showAllChains, const QList<bool>& showChainList, bool showCredibility, bool showCalib, bool showWiggle)
{
    Q_ASSERT(mPhase);
    GraphViewResults::updateCurvesToShow(showAllChains, showChainList, showCredibility, showCalib, showWiggle);
    
    /* --------------------first tab : posterior distrib----------------------------
     *
     *  Possible curves :
     *  - Post Distrib Alpha All Chains
     *  - Post Distrib Beta All Chains
     *  - HPD Alpha All Chains
     *  - HPD Beta All Chains
     *  - Duration
     *  - HPD Duration
     *  - Post Distrib Alpha i
     *  - Post Distrib Beta i
     * ------------------------------------------------
     */
    if (mCurrentTypeGraph == ePostDistrib && mCurrentVariable == eTheta) {
        mGraph->setTipYLab("");

        mGraph->setCurveVisible("Post Distrib Alpha All Chains", mShowAllChains);
        mGraph->setCurveVisible("Post Distrib Beta All Chains", mShowAllChains);
        mGraph->setCurveVisible("HPD Alpha All Chains", mShowAllChains);
        mGraph->setCurveVisible("HPD Beta All Chains", mShowAllChains);

        mGraph->setCurveVisible("Time Range", mShowAllChains);

        for (int i=0; i<mShowChainList.size(); ++i) {
            mGraph->setCurveVisible("Post Distrib Alpha " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Post Distrib Beta " + QString::number(i), mShowChainList.at(i));
        }

        mGraph->setTipXLab("t");
        mGraph->setTipYLab("");
        mGraph->setYAxisMode(GraphView::eHidden);
        mGraph->autoAdjustYScale(true); // do repaintGraph()

    }
    else if (mCurrentTypeGraph == ePostDistrib && mCurrentVariable == eDuration) {
        const GraphCurve* duration = mGraph->getCurve("Post Distrib Duration All Chains");

        if ( duration && !duration->mData.isEmpty()) {

            mGraph->setCurveVisible("Post Distrib Duration All Chains", mShowAllChains);
            mGraph->setCurveVisible("HPD Duration All Chains", mShowAllChains);

            for (int i=0; i<mShowChainList.size(); ++i)
                mGraph->setCurveVisible("Post Distrib Duration " + QString::number(i), mShowChainList.at(i));

            mGraph->setTipXLab("t");
            mGraph->setTipYLab("");
            mGraph->setYAxisMode(GraphView::eHidden);
            mGraph->autoAdjustYScale(true);
        }

    }
    /* ---------------- second tab : history plot--------------------------------
     *  - Alpha Trace i
     *  - Alpha Q1 i
     *  - Alpha Q2 i
     *  - Alpha Q3 i
     *  - Beta Trace i
     *  - Beta Q1 i
     *  - Beta Q2 i
     *  - Beta Q3 i
     * ------------------------------------------------ */
    else if (mCurrentTypeGraph == eTrace && mCurrentVariable == eTheta) {

        for (int i=0; i<mShowChainList.size(); ++i) {
            mGraph->setCurveVisible("Alpha Trace " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Alpha Q1 " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Alpha Q2 " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Alpha Q3 " + QString::number(i), mShowChainList.at(i));

            mGraph->setCurveVisible("Beta Trace " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Beta Q1 " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Beta Q2 " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Beta Q3 " + QString::number(i), mShowChainList.at(i));
        }

        mGraph->setTipXLab("iteration");
        mGraph->setTipYLab("t");
        mGraph->setYAxisMode(GraphView::eMinMax);
        mGraph->autoAdjustYScale(true);
    }
    else if (mCurrentTypeGraph == eTrace && mCurrentVariable == eDuration) {

        for (int i=0; i<mShowChainList.size(); ++i) {
            mGraph->setCurveVisible("Duration Trace " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Duration Q1 " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Duration Q2 " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Duration Q3 " + QString::number(i), mShowChainList.at(i));
        }

        mGraph->setTipXLab("iteration");
        mGraph->setTipYLab("t");
        mGraph->setYAxisMode(GraphView::eMinMax);
        mGraph->autoAdjustYScale(true);
    }
    repaint();
}

