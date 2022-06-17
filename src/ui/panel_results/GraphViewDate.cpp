/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

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
    //setItemTitle(tr("Data : %1").arg(mDate->mName));

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


void GraphViewDate::generateCurves(const graph_t typeGraph, const QVector<variable_t>& variableList, const Model* model)
{
    //qDebug()<<"GraphViewDate::generateCurves()";
    (void) model;
    GraphViewResults::generateCurves(typeGraph, variableList);

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

    QString resultsText = tr("Nothing to Display");
    QString resultsHTML = tr("Nothing to Display");
    if (variableList.contains(eDataTi)) {
        resultsText = ModelUtilities::dateResultsText(mDate);
        resultsHTML = ModelUtilities::dateResultsHTML(mDate);

    } else if (variableList.contains(eSigma)) {
        resultsText = ModelUtilities::sigmaTiResultsText(mDate);
        resultsHTML = ModelUtilities::sigmaTiResultsHTML(mDate);
    }

    setNumericalResults(resultsHTML, resultsText);

    /* ------------------------------------------------
     *  Are we working on calendar date or std dev ?
     * ------------------------------------------------
     */

    /* ------------------------------------------------
     *  First tab : Posterior distrib.
     * ------------------------------------------------
     */
    if (typeGraph == ePostDistrib && (variableList.contains(eDataTi) || variableList.contains(eSigma))) {

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
        if (variableList.contains(eDataTi)) {
            mGraph->setOverArrow(GraphView::eBothOverflow);
            mTitle = tr("Data : %1").arg(mDate->mName);

            mGraph->mLegendX = DateUtils::getAppSettingsFormatStr();
            mGraph->setFormatFunctX(nullptr);//DateUtils::convertToAppSettingsFormat);
            mGraph->setFormatFunctY(nullptr);

            //  Post Distrib All Chains
            GraphCurve curvePostDistrib = densityCurve(mDate->mTi.fullHisto(),
                                                         "Post Distrib All Chains",
                                                         color,
                                                         Qt::SolidLine,
                                                         Qt::NoBrush);

            mGraph->addCurve(curvePostDistrib);

            // Post Distrib Chain i
            if (!mDate->mTi.mChainsHistos.isEmpty())
                for (int i=0; i<mChains.size(); ++i) {
                    GraphCurve curvePostDistribChain = densityCurve(mDate->mTi.histoForChain(i),
                                                                            "Post Distrib Chain " + QString::number(i),
                                                                            Painting::chainColors.at(i),
                                                                            Qt::SolidLine,
                                                                            Qt::NoBrush);
                    mGraph->addCurve(curvePostDistribChain);
                }

            // HPD All Chains
            GraphCurve curveHPD = HPDCurve(mDate->mTi.mHPD,
                                                   "HPD All Chains",
                                                    color);
            mGraph->addCurve(curveHPD);

            // Calibration
            const QMap<double,double> formatedCalib = mDate->getFormatedCalibToShow();//getFormatedCalibMap();

            GraphCurve curveCalib = densityCurve(formatedCalib,
                                                         "Calibration",
                                                         QColor(150, 150, 150),
                                                         Qt::SolidLine,
                                                         Qt::NoBrush);
            mGraph->addCurve(curveCalib);

            // Wiggle
            GraphCurve curveWiggle = densityCurve(mDate->mWiggle.fullHisto(),
                                                          "Wiggle",
                                                          mColor,
                                                          Qt::DashLine,
                                                          Qt::NoBrush);
            mGraph->addCurve(curveWiggle);

            // Credibility (must be the last created curve because uses yMax!
            GraphCurve curveCred = sectionCurve(mDate->mTi.mCredibility,
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
         *  - Sigma
         *  -- Post Distrib All Chains
         *  -- Post Distrib Chain i
         * ------------------------------------------------
         */
        else if (variableList.contains(eSigma)) {
            mGraph->setOverArrow(GraphView::eNone);
            mTitle = tr("Individual Std : %1").arg(mDate->mName);

            mGraph->mLegendX = "";
            mGraph->setFormatFunctX(nullptr);//DateUtils::convertToAppSettingsFormat);
            mGraph->setFormatFunctY(nullptr);

            //  Post Distrib All Chains
            GraphCurve curvePostDistrib = densityCurve(mDate->mSigmaTi.fullHisto(),
                                                               "Post Distrib all Chains",
                                                               color,
                                                               Qt::SolidLine,
                                                               Qt::NoBrush);
            mGraph->addCurve(curvePostDistrib);

            // Post Distrib Chain i
            if (!mDate->mSigmaTi.mChainsHistos.isEmpty())
                for (int i=0; i<mChains.size(); ++i) {
                    GraphCurve curvePostDistribChain = densityCurve(mDate->mSigmaTi.histoForChain(i),
                                                                            "Post Distrib Chain " + QString::number(i),
                                                                            Painting::chainColors.at(i),
                                                                            Qt::SolidLine,
                                                                            Qt::NoBrush);
                    mGraph->addCurve(curvePostDistribChain);
                }
            // HPD All Chains
            GraphCurve curveHPD = HPDCurve(mDate->mSigmaTi.mHPD,
                                                   "HPD All Chains",
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
    else if (typeGraph == eTrace && (variableList.contains(eDataTi) || variableList.contains(eSigma))) {
        mGraph->mLegendX = tr("Iterations");
        mGraph->setFormatFunctX(nullptr);
        mGraph->setFormatFunctY(nullptr);//DateUtils::convertToAppSettingsFormat);

        if (variableList.contains(eDataTi)) {
            mTitle = tr("Data : %1").arg(mDate->mName);
            generateTraceCurves(mChains, &mDate->mTi);

        } else {
             mTitle = tr("Individual Std : %1").arg(mDate->mName);
             generateTraceCurves(mChains, &mDate->mSigmaTi);
        }

    }
    /* ------------------------------------------------
     *  Third tab : Acceptance rate.
     *  Possible curves (could be for theta or sigma):
     *  - Accept i
     *  - Accept Target
     * ------------------------------------------------
     */
    else if (typeGraph == eAccept && (variableList.contains(eDataTi) || variableList.contains(eSigma))) {
        mGraph->mLegendX = tr("Iterations");
        mGraph->setFormatFunctX(nullptr);
        mGraph->setFormatFunctY(nullptr);
        mGraph->autoAdjustYScale(true);
        if (variableList.contains(eDataTi)) {
            mTitle = tr("Data : %1").arg(mDate->mName);
            generateAcceptCurves(mChains, &mDate->mTi);
        } else {
            mTitle = tr("Individual Std : %1").arg(mDate->mName);
            generateAcceptCurves(mChains, &mDate->mSigmaTi);
        }

        //mGraph->addCurve( horizontalLine(44, "Accept Target", QColor(180, 10, 20), Qt::DashLine) );

    }

    /* ------------------------------------------------
     *  fourth tab : Autocorrelation
     *  Possible curves (could be for theta or sigma):
     *  - Correl i
     *  - Correl Limit Lower i
     *  - Correl Limit Upper i
     * ------------------------------------------------
     */
    else if (typeGraph == eCorrel && (variableList.contains(eDataTi) || variableList.contains(eSigma))) {
        mGraph->mLegendX = "";
        mGraph->setFormatFunctX(nullptr);
        mGraph->setFormatFunctY(nullptr);
        if (variableList.contains(eDataTi)) {
            mTitle = tr("Data : %1").arg(mDate->mName);
           generateCorrelCurves(mChains, &mDate->mTi);
        } else {
             mTitle = tr("Individual Std : %1").arg(mDate->mName);
             generateCorrelCurves(mChains, &mDate->mSigmaTi);
        }

        mGraph->setXScaleDivision(10, 10);
    }
    else {
        mTitle = tr("Data : %1").arg(mDate->mName);
        mGraph->resetNothingMessage();

    }

}

void GraphViewDate::updateCurvesToShow(bool showAllChains, const QList<bool>& showChainList, const QVector<variable_t>& variableList)
{
    GraphViewResults::updateCurvesToShow(showAllChains, showChainList, variableList);

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
        if (variableList.contains(eDataTi)) {

            const bool showCredibility = true;
            const bool showCalib = variableList.contains(eDataCalibrate);
            const bool showWiggle = variableList.contains(eDataWiggle);
            mGraph->setCurveVisible("Post Distrib All Chains", mShowAllChains);
            mGraph->setCurveVisible("HPD All Chains", mShowAllChains);
            mGraph->setCurveVisible("Credibility All Chains", mShowAllChains && showCredibility);
            mGraph->setCurveVisible("Calibration", showCalib);
            mGraph->setCurveVisible("Wiggle", showWiggle);
            for (int i=0; i<mShowChainList.size(); ++i)
                mGraph->setCurveVisible("Post Distrib Chain " + QString::number(i), mShowChainList[i]);

            mGraph->setTipXLab("t");
            mGraph->setYAxisMode(GraphView::eHidden);
            mGraph->showInfos(false);
            mGraph->clearInfos();

        }
        /* ------------------------------------------------
         *  Possible Curves :
         *  - Sigma
         *  -- Post Distrib All Chains
         *  -- Post Distrib Chain i
         * ------------------------------------------------
         */
        else if (variableList.contains(eSigma)) {
            mGraph->setCurveVisible("Post Distrib all Chains", mShowAllChains);
            for (int i=0; i<mShowChainList.size(); ++i)
                mGraph->setCurveVisible("Post Distrib Chain " + QString::number(i), mShowChainList[i]);

            mGraph->setCurveVisible("HPD All Chains", mShowAllChains);

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
        mGraph->autoAdjustYScale(true);
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
        mGraph->autoAdjustYScale(false);
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
