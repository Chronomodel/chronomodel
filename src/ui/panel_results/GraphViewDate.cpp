#include "GraphViewDate.h"
#include "GraphView.h"
#include "Date.h"
#include "Event.h"
#include "Painting.h"
#include "StdUtilities.h"
#include "QtUtilities.h"
#include "ModelUtilities.h"
#include <QtWidgets>
#include "../PluginAbstract.h"
#include "../GraphViewRefAbstract.h"



// Constructor / Destructor

GraphViewDate::GraphViewDate(QWidget *parent):GraphViewResults(parent),
mDate(nullptr),
mColor(Qt::blue)
{
    setMainColor(QColor(155, 155, 155));
    mGraph->setBackgroundColor(Qt::white);
}

GraphViewDate::~GraphViewDate()
{
    mDate = nullptr;
}

void GraphViewDate::setDate(Date* date)
{
    Q_ASSERT(date);
    mDate = date;
    setItemTitle(tr("Data : %1").arg(mDate->mName));

    update();
}

void GraphViewDate::setColor(const QColor& color)
{
    mColor = color;
    setItemColor(mColor);
    update();
}

void GraphViewDate::paintEvent(QPaintEvent* e)
{
    GraphViewResults::paintEvent(e);
}

// not used since v1.4
//QColor GraphViewDate::getEventColor()
//{
//    return mDate->getEventColor();
//}

void GraphViewDate::generateCurves(TypeGraph typeGraph, Variable variable)
{
    Q_ASSERT(mDate);
    //qDebug()<<"GraphViewDate::generateCurves()";
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

    QColor color = mDate->mColor;
    QPen defaultPen;
    defaultPen.setWidthF(1);
    defaultPen.setStyle(Qt::SolidLine);
    QString resultsText = ModelUtilities::dateResultsText(mDate);
    QString resultsHTML = ModelUtilities::dateResultsHTML(mDate);
    setNumericalResults(resultsHTML, resultsText);

    /* ------------------------------------------------
     *  Are we working on calendar date or std dev ?
     * ------------------------------------------------
     */
    MHVariable* variableDate = &(mDate->mTheta);
    if (variable == eTheta)
        variableDate = &(mDate->mTheta);

    else if(variable == eSigma)
        variableDate = &(mDate->mSigma);

    /* ------------------------------------------------
     *  First tab : Posterior distrib.
     * ------------------------------------------------
     */
    if (typeGraph == ePostDistrib && (variable == eTheta || variable == eSigma)) {

        /* ------------------------------------------------
         *  Possible Curves :
         *  - Post Distrib All Chains
         *  - Post Distrib Chain i
         *  - HPD All Chains
         *  - Credibility All Chains
         *  - Calibration
         *  - Wiggle
         * ------------------------------------------------
         */
        if (variable == eTheta) {
            mGraph->setOverArrow(GraphView::eBothOverflow);
            mTitle = tr("Data : %1").arg(mDate->mName);


            mGraph->mLegendX = DateUtils::getAppSettingsFormatStr();
            mGraph->setFormatFunctX(nullptr);//DateUtils::convertToAppSettingsFormat);
            mGraph->setFormatFunctY(nullptr);


            //  Post Distrib All Chains
            GraphCurve curvePostDistrib = generateDensityCurve(variableDate->fullHisto(),
                                                         "Post Distrib All Chains",
                                                         color,
                                                         Qt::SolidLine,
                                                         Qt::NoBrush);
            mGraph->addCurve(curvePostDistrib);

            // Post Distrib Chain i
            if (!variableDate->mChainsHistos.isEmpty())
                for (int i=0; i<mChains.size(); ++i) {
                    GraphCurve curvePostDistribChain = generateDensityCurve(variableDate->histoForChain(i),
                                                                            "Post Distrib Chain " + QString::number(i),
                                                                            Painting::chainColors.at(i),
                                                                            Qt::SolidLine,
                                                                            Qt::NoBrush);
                    mGraph->addCurve(curvePostDistribChain);
                }

            // HPD All Chains
            GraphCurve curveHPD = generateHPDCurve(variableDate->mHPD,
                                                   "HPD All Chains",
                                                    color);
            mGraph->addCurve(curveHPD);

            // Calibration
            const QMap<double,double> formatedCalib = mDate->getFormatedCalibMap();

            GraphCurve curveCalib = generateDensityCurve(formatedCalib,
                                                         "Calibration",
                                                         QColor(150, 150, 150),
                                                         Qt::SolidLine,
                                                         Qt::NoBrush);
            mGraph->addCurve(curveCalib);

            // Wiggle
            GraphCurve curveWiggle = generateDensityCurve(mDate->mWiggle.fullHisto(),
                                                          "Wiggle",
                                                          mColor,
                                                          Qt::DashLine,
                                                          Qt::NoBrush);
            mGraph->addCurve(curveWiggle);

            // Credibility (must be the last created curve because uses yMax!
            GraphCurve curveCred = generateSectionCurve(variableDate->mCredibility,
                                                            "Credibility All Chains",
                                                            color);
            mGraph->addCurve(curveCred);

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


            mGraph->setYAxisMode(GraphView::eHidden);
        }

        /* ------------------------------------------------
         *  Possible Curves :
         *  - Sigma All Chains
         *  - Sigma Chain i
         * ------------------------------------------------
         */
        else if (variable == eSigma) {
            mGraph->setOverArrow(GraphView::eNone);
            mTitle = tr("Individual Std : %1").arg(mDate->mName);

            mGraph->mLegendX = "";
            mGraph->setFormatFunctX(nullptr);//DateUtils::convertToAppSettingsFormat);
            mGraph->setFormatFunctY(nullptr);

            //  Post Distrib All Chains
            GraphCurve curvePostDistrib = generateDensityCurve(variableDate->fullHisto(),
                                                               "Sigma all Chains",
                                                               color,
                                                               Qt::SolidLine,
                                                               Qt::NoBrush);
            mGraph->addCurve(curvePostDistrib);

            // Post Distrib Chain i
            if (!variableDate->mChainsHistos.isEmpty())
                for (int i=0; i<mChains.size(); ++i) {
                    GraphCurve curvePostDistribChain = generateDensityCurve(variableDate->histoForChain(i),
                                                                            "Sigma for Chain " + QString::number(i),
                                                                            Painting::chainColors.at(i),
                                                                            Qt::SolidLine,
                                                                            Qt::NoBrush);
                    mGraph->addCurve(curvePostDistribChain);
                }
            // HPD All Chains
            GraphCurve curveHPD = generateHPDCurve(variableDate->mHPD,
                                                   "Sigma HPD All Chains",
                                                    color);
            mGraph->addCurve(curveHPD);

            mGraph->setYAxisMode(GraphView::eHidden);
        }

    }
    /* ------------------------------------------------
     *  Second tab : History plots.
     *  Possible Curves (could be for theta or sigma):
     *  - Trace i
     *  - Q1 i
     *  - Q2 i
     *  - Q3 i
     * ------------------------------------------------
     */
    else if (typeGraph == eTrace && (variable == eTheta || variable == eSigma)) {
        mGraph->mLegendX = tr("Iterations");
        mGraph->setFormatFunctX(nullptr);
        mGraph->setFormatFunctY(nullptr);//DateUtils::convertToAppSettingsFormat);
        if (variable == eTheta)
            mTitle = tr("Data : %1").arg(mDate->mName);
        else
             mTitle = tr("Individual Std : %1").arg(mDate->mName);

        generateTraceCurves(mChains, variableDate);
    }
    /* ------------------------------------------------
     *  Third tab : Acceptance rate.
     *  Possible curves (could be for theta or sigma):
     *  - Accept i
     *  - Accept Target
     * ------------------------------------------------
     */
    else if (typeGraph == eAccept && (variable == eTheta || variable == eSigma)) {
        mGraph->mLegendX = tr("Iterations");
        mGraph->setFormatFunctX(nullptr);
        mGraph->setFormatFunctY(nullptr);
        mGraph->autoAdjustYScale(true);
        if (variable == eTheta)
            mTitle = tr("Data : %1").arg(mDate->mName);
        else
             mTitle = tr("Individual Std : %1").arg(mDate->mName);

        mGraph->addCurve( generateHorizontalLine(44, "Accept Target", QColor(180, 10, 20), Qt::DashLine) );
        generateAcceptCurves(mChains, variableDate);
    }

    /* ------------------------------------------------
     *  fourth tab : Autocorrelation
     *  Possible curves (could be for theta or sigma):
     *  - Correl i
     *  - Correl Limit Lower i
     *  - Correl Limit Upper i
     * ------------------------------------------------
     */
    else if (typeGraph == eCorrel && (variable == eTheta || variable == eSigma)) {
        mGraph->mLegendX = "";
        mGraph->setFormatFunctX(nullptr);
        mGraph->setFormatFunctY(nullptr);
        if (variable == eTheta)
            mTitle = tr("Data : %1").arg(mDate->mName);
        else
             mTitle = tr("Individual Std : %1").arg(mDate->mName);

        generateCorrelCurves(mChains, variableDate);
        mGraph->setXScaleDivision(10, 10);
    }
    else {
        mTitle = tr("Data : %1").arg(mDate->mName);
        mGraph->resetNothingMessage();

    }

}

void GraphViewDate::updateCurvesToShow(bool showAllChains, const QList<bool>& showChainList, bool showCredibility, bool showCalib, bool showWiggle)
{
    GraphViewResults::updateCurvesToShow(showAllChains, showChainList, showCredibility, showCalib, showWiggle);
    
    /* --------------------First Tab : Posterior distrib.------------*/

    if (mCurrentTypeGraph == ePostDistrib) {
        mGraph->setTipYLab("");
        /* ------------------------------------------------
         *  Possible Curves :
         *  - Post Distrib All Chains
         *  - Post Distrib Chain i
         *  - HPD All Chains
         *  - Credibility All Chains
         *  - Calibration
         *  - Wiggle
         * ------------------------------------------------
         */
        if (mCurrentVariable == eTheta) {
            
            mGraph->setCurveVisible("Post Distrib All Chains", mShowAllChains);
            mGraph->setCurveVisible("HPD All Chains", mShowAllChains);
            mGraph->setCurveVisible("Credibility All Chains", mShowAllChains && mShowCredibility);
            mGraph->setCurveVisible("Calibration", mShowCalib);
            mGraph->setCurveVisible("Wiggle", mShowWiggle);
            for (int i=0; i<mShowChainList.size(); ++i)
                mGraph->setCurveVisible("Post Distrib Chain " + QString::number(i), mShowChainList[i]);
            
            mGraph->setTipXLab("t");
            mGraph->setYAxisMode(GraphView::eHidden);
            mGraph->showInfos(false);
            mGraph->clearInfos();

        }
        /* ------------------------------------------------
         *  Possible Curves :
         *  - Sigma All Chains
         *  - Sigma Chain i
         * ------------------------------------------------
         */
        else if (mCurrentVariable == eSigma) {
            mGraph->setCurveVisible("Sigma all Chains", mShowAllChains);
            for (int i=0; i<mShowChainList.size(); ++i)
                mGraph->setCurveVisible("Sigma for Chain " + QString::number(i), mShowChainList[i]);

            mGraph->setCurveVisible("Sigma HPD All Chains", mShowAllChains);

            mGraph->setTipXLab(tr("sigma"));
            mGraph->setYAxisMode(GraphView::eHidden);
        }
        mGraph->autoAdjustYScale(true);
    }
    /* ------------------Second tab : History plots.------------------------------
     *  Possible Curves (could be for theta or sigma):
     *  - Trace i
     *  - Q1 i
     *  - Q2 i
     *  - Q3 i
     * ------------------------------------------------
     */
    else if (mCurrentTypeGraph == eTrace) {
        for (int i=0; i<mShowChainList.size(); ++i) {
            mGraph->setCurveVisible("Trace " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Q1 " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Q2 " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Q3 " + QString::number(i), mShowChainList.at(i));
        }

        mGraph->setTipXLab(tr("Iteration"));
        mGraph->setTipYLab("t");

        mGraph->setYAxisMode(GraphView::eMinMaxHidden);
        mGraph->showInfos(true);
        mGraph->autoAdjustYScale(true); // do  repaintGraph()
    }
    /* -----------------------Third tab : Acceptance rate.-------------------------
     *  Possible curves (could be for theta or sigma):
     *  - Accept i
     *  - Accept Target
     * ------------------------------------------------ */
    else if (mCurrentTypeGraph == eAccept) {

        mGraph->setCurveVisible("Accept Target", true);
        for (int i=0; i<mShowChainList.size(); ++i)
            mGraph->setCurveVisible("Accept " + QString::number(i), mShowChainList.at(i));
        
        mGraph->setTipXLab(tr("Iteration"));
        mGraph->setTipYLab(tr("Rate"));

        mGraph->setYAxisMode(GraphView::eMinMax );
        mGraph->showInfos(false);
        mGraph->clearInfos();
        mGraph->autoAdjustYScale(false); // do  repaintGraph()
        mGraph->setRangeY(0, 100); // do repaintGraph() !!
    }
    
    /* -------------------- fourth tab : Autocorrelation----------------------------
     *  Possible curves (could be for theta or sigma):
     *  - Correl i
     *  - Correl Limit Lower i
     *  - Correl Limit Upper i
     * ------------------------------------------------
     */
    else if (mCurrentTypeGraph == eCorrel) {
        for (int i=0; i<mShowChainList.size(); ++i) {
            mGraph->setCurveVisible("Correl " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Correl Limit Lower " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Correl Limit Upper " + QString::number(i), mShowChainList.at(i));
        }
        mGraph->setTipXLab("h");
        mGraph->setTipYLab(tr("Value"));
        mGraph->setYAxisMode(GraphView::eMinMax);
        mGraph->showInfos(false);
        mGraph->clearInfos();
        mGraph->autoAdjustYScale(false);
        mGraph->setRangeY(-1, 1);
    }
   update();
}
