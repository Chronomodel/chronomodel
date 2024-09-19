/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2024

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
#include "Painting.h"
#include "ModelUtilities.h"
#include "StdUtilities.h"

#include <QtWidgets>


GraphViewDate::GraphViewDate(QWidget *parent):
    GraphViewResults(parent),
    mDate(nullptr)
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
    update();
}

void GraphViewDate::updateColor(const QColor &color)
{
    setItemColor(color);
    update();
}

void GraphViewDate::generateCurves(const graph_t typeGraph, const QList<variable_t>& variableList)
{
    GraphViewResults::generateCurves(typeGraph, variableList);

    /* ------------------------------------------------
     *  Reset the graph object settings
     * ------------------------------------------------
     */
    mGraph->removeAllCurves();
    mGraph->reserveCurves(6);

    mGraph->remove_all_zones();
    mGraph->clearInfos();
    mGraph->resetNothingMessage();
    mGraph->setOverArrow(GraphView::eNone);

    QColor color = mDate->mColor;
    QPen defaultPen;
    defaultPen.setWidthF(1);
    defaultPen.setStyle(Qt::SolidLine);

    QString resultsHTML = tr("Nothing to Display");

    if (variableList.contains(eDataTi)) {
        resultsHTML = ModelUtilities::dateResultsHTML(mDate);

    } else if (variableList.contains(eSigma)) {
        resultsHTML = ModelUtilities::sigmaTiResultsHTML(mDate);
    }

    setNumericalResults(resultsHTML);

     /* ------------------------------------------------
     *  First tab : Posterior distrib.
     * ------------------------------------------------
     */
    if (typeGraph == ePostDistrib) {

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
            mTitle = tr("Data : %1").arg(mDate->getQStringName());

            mGraph->mLegendX = DateUtils::getAppSettingsFormatStr();
            mGraph->setFormatFunctX(nullptr);//DateUtils::convertToAppSettingsFormat);
            mGraph->setFormatFunctY(nullptr);

            // Calibration
            const std::map<double,double> &formatedCalib = mDate->getFormatedCalibToShow();
            const double max_formatedCalib = map_max(formatedCalib)->second;

            const GraphCurve &curveCalib = densityCurve(formatedCalib,
                                                        "Calibration",
                                                        QColor(150, 150, 150),
                                                        Qt::SolidLine,
                                                        Qt::NoBrush);
            mGraph->add_curve(curveCalib);

            // HPD All Chains
            //usefull for fixed value and display calibration
            const std::map<double, double> &norm = mDate->mTi.mFormatedHPD.size() == 1 ? std::map<double, double> {{mDate->mTi.mFormatedHPD.begin()->first, max_formatedCalib}} : mDate->mTi.mFormatedHPD;
            const GraphCurve &curveHPD = HPDCurve(norm,
                                                  "HPD All Chains",
                                                  color);
            mGraph->add_curve(curveHPD);

            //  Post Distrib All Chains
            const std::map<double, double> &normPostDistrib = mDate->mTi.mFormatedHisto.size() == 1 ? std::map<double, double> {{mDate->mTi.mFormatedHisto.begin()->first, max_formatedCalib}} : mDate->mTi.mFormatedHisto;
            const GraphCurve &curvePostDistrib = densityCurve(normPostDistrib,
                                                              "Post Distrib All Chains",
                                                              color,
                                                              Qt::SolidLine,
                                                              Qt::NoBrush);

            mGraph->add_curve(curvePostDistrib);

            // Post Distrib Chain i
            if (!mDate->mTi.mChainsHistos.empty())
                for (size_t i=0; i<mChains.size(); ++i) {
                    const std::map<double, double> &normPostDistribChain = mDate->mTi.mChainsHistos.at(i).size() == 1 ? std::map<double, double> {{mDate->mTi.mChainsHistos.at(i).begin()->first, max_formatedCalib}} : mDate->mTi.mChainsHistos.at(i);
                    const GraphCurve &curvePostDistribChain = densityCurve(normPostDistribChain,
                                                                           "Post Distrib Chain " + QString::number(i),
                                                                           Painting::chainColors.at(i),
                                                                           Qt::SolidLine,
                                                                           Qt::NoBrush);
                    mGraph->add_curve(curvePostDistribChain);
                    if (!mDate->mWiggle.mChainsHistos.empty()) {
                        const std::map<double, double> &normPostWiggleChain = mDate->mWiggle.mChainsHistos.at(i).size() == 1 ? std::map<double, double> {{mDate->mWiggle.mChainsHistos.at(i).begin()->first, max_formatedCalib}} : mDate->mWiggle.mChainsHistos.at(i);

                        const GraphCurve &curveWiggle = densityCurve(normPostWiggleChain,
                                                                     "Wiggle Post Distrib Chain " + QString::number(i),
                                                                     Painting::chainColors.at(i),
                                                                     Qt::DashLine,
                                                                     Qt::NoBrush);
                        mGraph->add_curve(curveWiggle);
                    }

                }

            // ---- Wiggle

            //  Post Distrib All Chains
            const std::map<double, double> &normPostWiggleChain = mDate->mWiggle.mFormatedHisto.size() == 1 ? std::map<double, double> {{mDate->mWiggle.mFormatedHisto.begin()->first, max_formatedCalib}} : mDate->mWiggle.mFormatedHisto;

            const GraphCurve &curveWiggle = densityCurve( normPostWiggleChain,
                                                          "Wiggle Post Distrib All Chains",
                                                          mItemColor,
                                                          Qt::DashLine,
                                                          Qt::NoBrush);
            mGraph->add_curve(curveWiggle);

            // Calibration
            const std::map<double, double> &formatedWiggle = mDate->getFormatedWiggleCalibToShow();

            const GraphCurve &curveWiggleCal = densityCurve(formatedWiggle,
                                                        "Wiggle Calibration",
                                                        QColor(150, 150, 150),
                                                        Qt::DashLine,
                                                        Qt::NoBrush);
            mGraph->add_curve(curveWiggleCal);


            // Credibility (must be the last created curve because uses yMax!
            GraphCurve curveCred = topLineSection(mDate->mTi.mFormatedCredibility,
                                                            "Credibility All Chains",
                                                            color);
            mGraph->add_curve(curveCred);

            // ------------------------------------------------------------
            //  Add zones outside study period
            // ------------------------------------------------------------
            const GraphZone zoneMin (-INFINITY, mSettings.getTminFormated());
            mGraph->add_zone(zoneMin);

            const GraphZone zoneMax (mSettings.getTmaxFormated(), INFINITY);
            mGraph->add_zone(zoneMax);

            /*GraphZone zoneMin;
            zoneMin.mXStart = -INFINITY;
            zoneMin.mXEnd = mSettings.getTminFormated();
            zoneMin.mColor = QColor(217, 163, 69);
            zoneMin.mColor.setAlpha(35);
            zoneMin.mText = tr("Outside study period");
            mGraph->add_zone(zoneMin);

            GraphZone zoneMax;
            zoneMax.mXStart = mSettings.getTmaxFormated();
            zoneMax.mXEnd = INFINITY;
            zoneMax.mColor = QColor(217, 163, 69);
            zoneMax.mColor.setAlpha(35);
            zoneMax.mText = tr("Outside study period");
            mGraph->add_zone(zoneMax);*/

            //mGraph->setYAxisMode(GraphView::eHidden);
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
            mTitle = tr("Individual Std : %1").arg(mDate->getQStringName());

            mGraph->mLegendX = "";
            mGraph->setFormatFunctX(nullptr);
            mGraph->setFormatFunctY(nullptr);

            //  Post Distrib All Chains
            const GraphCurve &curvePostDistrib = densityCurve(mDate->mSigmaTi.fullHisto(),
                                                               "Post Distrib all Chains",
                                                               color,
                                                               Qt::SolidLine,
                                                               Qt::NoBrush);
            mGraph->add_curve(curvePostDistrib);

            // Post Distrib Chain i
            if (!mDate->mSigmaTi.mChainsHistos.empty())
                for (size_t i=0; i<mChains.size(); ++i) {
                    const GraphCurve &curvePostDistribChain = densityCurve(mDate->mSigmaTi.histoForChain(i),
                                                                            "Post Distrib Chain " + QString::number(i),
                                                                            Painting::chainColors.at(i),
                                                                            Qt::SolidLine,
                                                                            Qt::NoBrush);
                    mGraph->add_curve(curvePostDistribChain);
                }
            // HPD All Chains
            const GraphCurve &curveHPD = HPDCurve(mDate->mSigmaTi.mFormatedHPD,
                                                   "HPD All Chains",
                                                    color);
            mGraph->add_curve(curveHPD);
            // Credibility (must be the last created curve because uses yMax!
            GraphCurve curveCred = topLineSection(mDate->mSigmaTi.mFormatedCredibility,
                                                            "Credibility All Chains",
                                                            color);
            mGraph->add_curve(curveCred);
            mGraph->setYAxisMode(GraphView::eHidden);
        }

    }
    /* ------------------------------------------------
     *  Second tab : History plots.
     *  Possible Curves (could be for ti or sigma):
     *  - Trace i
     *  - Q1 i
     *  - Q2 i
     *  - Q3 i
     * ------------------------------------------------
     */
    else if (typeGraph == eTrace) {
        mGraph->mLegendX = tr("Iterations");
        mGraph->setFormatFunctX(nullptr);
        mGraph->setFormatFunctY(nullptr);

        if (variableList.contains(eDataTi) && mDate->mTi.mSamplerProposal!= MHVariable::eFixe) {
            mTitle = tr("Data : %1").arg(mDate->getQStringName());
            generateTraceCurves(mChains, &mDate->mTi);

        } else if (variableList.contains(eSigma) && mDate->mSigmaTi.mSamplerProposal!= MHVariable::eFixe) {
             mTitle = tr("Individual Std : %1").arg(mDate->getQStringName());
             generateTraceCurves(mChains, &mDate->mSigmaTi);
        }

    }
    /* ------------------------------------------------
     *  Third tab : Acceptance rate.
     *  Possible curves (could be for ti or sigma):
     *  - Accept i
     *  - Accept Target
     * ------------------------------------------------
     */
    else if (typeGraph == eAccept) {
        mGraph->mLegendX = tr("Iterations");
        mGraph->setFormatFunctX(nullptr);
        mGraph->setFormatFunctY(nullptr);
        mGraph->autoAdjustYScale(true);

        if (variableList.contains(eDataTi) && mDate->mTi.mSamplerProposal!= MHVariable::eFixe) {
            mTitle = tr("Data : %1").arg(mDate->getQStringName());
            generateAcceptCurves(mChains, &mDate->mTi);

        } else if (variableList.contains(eSigma) && mDate->mSigmaTi.mSamplerProposal!= MHVariable::eFixe) {
            mTitle = tr("Individual Std : %1").arg(mDate->getQStringName());
            generateAcceptCurves(mChains, &mDate->mSigmaTi);
        }
    }

    /* ------------------------------------------------
     *  fourth tab : Autocorrelation
     *  Possible curves (could be for theta or sigma):
     *  - Correl i
     *  - Correl Limit Lower i
     *  - Correl Limit Upper i
     * ------------------------------------------------
     */
    else if (typeGraph == eCorrel) {
        mGraph->mLegendX = "";
        mGraph->setFormatFunctX(nullptr);
        mGraph->setFormatFunctY(nullptr);

        if (variableList.contains(eDataTi) && mDate->mTi.mSamplerProposal!= MHVariable::eFixe) {
            mTitle = tr("Data : %1").arg(mDate->getQStringName());
           generateCorrelCurves(mChains, &mDate->mTi);

        } else if (variableList.contains(eSigma) && mDate->mSigmaTi.mSamplerProposal!= MHVariable::eFixe) {
             mTitle = tr("Individual Std : %1").arg(mDate->getQStringName());
             generateCorrelCurves(mChains, &mDate->mSigmaTi);
        }

        mGraph->setXScaleDivision(10, 10);
    }
    else {
        mTitle = tr("Data : %1").arg(mDate->getQStringName());
        mGraph->resetNothingMessage();

    }

}

void GraphViewDate::updateCurvesToShow(bool showAllChains, const QList<bool>& showChainList, const QList<variable_t> &variableList)
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

            const bool showCalib = variableList.contains(eDataCalibrate);
            const bool showWiggle = variableList.contains(eDataWiggle);
            mGraph->setCurveVisible("Post Distrib All Chains", mShowAllChains);
            mGraph->setCurveVisible("Wiggle Post Distrib All Chains", mShowAllChains && showWiggle);
            mGraph->setCurveVisible("HPD All Chains", mShowAllChains);
            mGraph->setCurveVisible("Credibility All Chains", mShowAllChains && mShowVariableList.contains(eCredibility));
            mGraph->setCurveVisible("Calibration", showCalib);
            mGraph->setCurveVisible("Wiggle Calibration", showWiggle && showCalib);
            for (int i=0; i<mShowChainList.size(); ++i) {
                mGraph->setCurveVisible("Post Distrib Chain " + QString::number(i), mShowChainList[i]);
                mGraph->setCurveVisible("Wiggle Post Distrib Chain " + QString::number(i), mShowChainList[i] && showWiggle);
            }

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
            for (int i=0; i<mShowChainList.size(); ++i) {
                mGraph->setCurveVisible("Post Distrib Chain " + QString::number(i), mShowChainList[i]);
                mGraph->setCurveVisible("Wiggle Post Distrib Chain " + QString::number(i), mShowChainList[i]);
            }

            mGraph->setCurveVisible("HPD All Chains", mShowAllChains);
            mGraph->setCurveVisible("Credibility All Chains", mShowAllChains && mShowVariableList.contains(eCredibility));

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

        mGraph->setTipXLab(tr("Iteration"));
        mGraph->setTipYLab("t");

        mGraph->setYAxisMode(GraphView::eMinMaxHidden);
        mGraph->showInfos(false);
        mGraph->autoAdjustYScale(true);

        for (int i=0; i<mShowChainList.size(); ++i) {
            mGraph->setCurveVisible("Trace " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Q1 " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Q2 " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Q3 " + QString::number(i), mShowChainList.at(i));
        }

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

    /* -------------------- Fourth tab : Autocorrelation----------------------------
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
