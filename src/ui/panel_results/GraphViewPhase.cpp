/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2022

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

#include "GraphViewPhase.h"

#include "GraphView.h"
#include "Phase.h"
#include "Painting.h"
#include "StdUtilities.h"
#include "QtUtilities.h"
#include "DateUtils.h"
#include "ModelUtilities.h"
//#include "MainWindow.h"
//#include "Button.h"
#include <QtWidgets>

// Constructor / Destructor

GraphViewPhase::GraphViewPhase(QWidget *parent):GraphViewResults(parent),
mPhase(nullptr)
{
    setMainColor(Painting::borderDark);
    mGraph->setBackgroundColor(QColor(210, 210, 210));
}

GraphViewPhase::~GraphViewPhase()
{
    mPhase = nullptr;
}

void GraphViewPhase::setPhase(Phase* phase)
{
    Q_ASSERT(phase);

    mPhase = phase;
    setItemColor(mPhase->mColor);
}

void GraphViewPhase::paintEvent(QPaintEvent* e)
{
    GraphViewResults::paintEvent(e);
}

void GraphViewPhase::resizeEvent(QResizeEvent* )
{
    updateLayout();
}

void GraphViewPhase::generateCurves(const graph_t typeGraph, const QVector<variable_t>& variableList, const Model* model)
{
    Q_ASSERT(mPhase);
    (void) model;
    GraphViewResults::generateCurves(typeGraph, variableList);

    mGraph->removeAllCurves();
    mGraph->reserveCurves(9);

    mGraph->removeAllZones();
    mGraph->clearInfos();
    mGraph->resetNothingMessage();

    QPen defaultPen;
    defaultPen.setWidthF(1.);
    defaultPen.setStyle(Qt::SolidLine);

    QColor color = mPhase->mColor;

    QString resultsText = tr("Nothing to Display");
    QString resultsHTML = tr("Nothing to Display");
    if (mCurrentVariableList.contains(eBeginEnd)) {
        resultsText = ModelUtilities::phaseResultsText(mPhase);
        resultsHTML = ModelUtilities::phaseResultsHTML(mPhase);

    } else if (mCurrentVariableList.contains(eTempo)) {
        resultsText = ModelUtilities::tempoResultsText(mPhase);
        resultsHTML = ModelUtilities::tempoResultsHTML(mPhase);

    } else if (mCurrentVariableList.contains(eDuration)) {
        resultsText = ModelUtilities::durationResultsText(mPhase);
        resultsHTML = ModelUtilities::durationResultsHTML(mPhase);

    } else if (mCurrentVariableList.contains(eActivity)) {
        resultsText = ModelUtilities::activityResultsText(mPhase);
        resultsHTML = ModelUtilities::activityResultsHTML(mPhase);
    }
    setNumericalResults(resultsHTML, resultsText);

    mGraph->setOverArrow(GraphView::eNone);

    /* -------------first tab : posterior distrib-----------------------------------
     *  Possible curves :
     *  - Begin-End
     *  -- Post Distrib Begin All Chains
     *  -- Post Distrib End All Chains
     *  -- HPD Begin All Chains
     *  -- HPD End All Chains
     *  -- Post Distrib Begin Chain i
     *  -- Post Distrib End Chain i
     *  - Time Range
     *
     * ------------------------------------------------  */
    if (typeGraph == ePostDistrib && mCurrentVariableList.contains(eBeginEnd)) {
        mGraph->mLegendX = DateUtils::getAppSettingsFormatStr();
        mGraph->mLegendY = "";
        mGraph->setFormatFunctX(nullptr);//DateUtils::convertToAppSettingsFormat);
        mGraph->setFormatFunctY(nullptr);
        mGraph->setYAxisMode(GraphView::eMinMaxHidden);

        mTitle = tr("Phase : %1").arg(mPhase->mName);
        QMap<double, double> &alpha = mPhase->mAlpha.mHisto;
        QMap<double, double> &beta = mPhase->mBeta.mHisto;

        QMap<double, double> &alphaHPD = mPhase->mAlpha.mHPD;
        QMap<double, double> &betaHPD = mPhase->mBeta.mHPD;
        /*
         * Detection of one Bound used as boundary != is xor
         * If there is two Bound, the both are egal to 1, thus nothing to do
         */
        const bool alphaIsBound = (alpha.size()==1);
        const bool betaIsBound = (beta.size()==1);
        double normPdf = 1.;
        if (alphaIsBound) {
            normPdf = map_max_value(beta);
            alpha[alpha.firstKey()] =  normPdf;
            alphaHPD[alphaHPD.firstKey()] = normPdf;

        } else if (betaIsBound) {
            normPdf = map_max_value(alpha);
            beta[beta.firstKey()] = normPdf;
            betaHPD[betaHPD.firstKey()] = normPdf;
        }

        const GraphCurve &curveBegin = densityCurve(alpha, "Post Distrib Begin All Chains", color, Qt::DotLine);
        const QColor colorEnd = mPhase->mColor.darker(170);


        const GraphCurve &curveEnd = densityCurve(beta, "Post Distrib End All Chains", colorEnd, Qt::DashLine);
        color.setAlpha(255); // set mBrush to fill
        const GraphCurve &curveBeginHPD = HPDCurve(alphaHPD, "HPD Begin All Chains", color);

        const GraphCurve &curveEndHPD = HPDCurve(betaHPD, "HPD End All Chains", colorEnd);

        mGraph->addCurve(curveBegin);
        mGraph->addCurve(curveEnd);


        mGraph->addCurve(curveBeginHPD);
        mGraph->addCurve(curveEndHPD);

        mGraph->setOverArrow(GraphView::eBothOverflow);

        GraphCurve curveTimeRange = topLineSection(mPhase->getFormatedTimeRange(), "Time Range", color);
        mGraph->addCurve(curveTimeRange);

        /* ------------------------------------------------------------
        *   Add zones Outside Study Period
        * ------------------------------------------------------------*/

        const GraphZone zoneMin (-INFINITY, mSettings.getTminFormated());
        mGraph->addZone(zoneMin);

        const GraphZone zoneMax (mSettings.getTmaxFormated(), INFINITY);
        mGraph->addZone(zoneMax);

        if (!mPhase->mAlpha.mChainsHistos.isEmpty())
            for (auto i=0; i<mChains.size(); ++i) {
                QMap<double, double> &alpha_i = mPhase->mAlpha.mChainsHistos[i];
                QMap<double, double> &beta_i = mPhase->mBeta.mChainsHistos[i];
                if (alphaIsBound) {
                    normPdf = map_max_value(beta);
                    alpha_i[alpha_i.firstKey()] =  normPdf;

                } else if (betaIsBound) {
                    normPdf = map_max_value(alpha);
                    beta_i[beta_i.firstKey()] = normPdf;
                  }

                const GraphCurve &curveBegin = densityCurve(alpha_i,
                                                     "Post Distrib Begin Chain " + QString::number(i),
                                                     Painting::chainColors.at(i), Qt::DotLine);

                const GraphCurve &curveEnd = densityCurve(beta_i,
                                                   "Post Distrib End Chain " + QString::number(i),
                                                   Painting::chainColors.at(i).darker(170), Qt::DashLine);
                mGraph->addCurve(curveBegin);
                mGraph->addCurve(curveEnd);
            }


    } else if (typeGraph == ePostDistrib && mCurrentVariableList.contains(eTempo)) {

        if (!mPhase->mTempo.isEmpty()) {
            mGraph->mLegendX = DateUtils::getAppSettingsFormatStr();
            mGraph->mLegendY = "Events";
            mGraph->setFormatFunctX(nullptr);
            mGraph->setFormatFunctY(nullptr);
            mGraph->setYAxisMode(GraphView::eAllTicks);

            mGraph->setTipXLab("t");
            mGraph->setTipYLab("n");
            mGraph->setYAxisMode(GraphView::eAllTicks);
            mGraph->autoAdjustYScale(true);

            mTitle = tr("Phase Tempo : %1").arg(mPhase->mName);

            GraphCurve curveTempo = densityCurve(mPhase->mTempo,
                                                 "Post Distrib All Chains",
                                                 color.darker(), Qt::SolidLine);
            curveTempo.mIsRectFromZero = false;

            auto brushColor = color;
            brushColor.setAlpha(30);
            const GraphCurve &curveTempoEnv = shapeCurve(mPhase->mTempoInf, mPhase->mTempoSup,
                                                  "Post Distrib Env All Chains",
                                                  color, Qt::CustomDashLine, brushColor);

            mGraph->addCurve(curveTempoEnv);
            mGraph->addCurve(curveTempo);

            mGraph->setOverArrow(GraphView::eBothOverflow);

            /* ------------------------------------------------------------
        *   Add zones outside study period
        * ------------------------------------------------------------*/

            const GraphZone zoneMin (-INFINITY, mSettings.getTminFormated());
            mGraph->addZone(zoneMin);

            const GraphZone zoneMax (mSettings.getTmaxFormated(), INFINITY);
            mGraph->addZone(zoneMax);

        } else
            mGraph->resetNothingMessage();


    } else if (typeGraph == ePostDistrib && mCurrentVariableList.contains(eActivity)) {

        if (!mPhase->mActivity.isEmpty()) {
            mGraph->mLegendX = DateUtils::getAppSettingsFormatStr();
            mGraph->mLegendY = "";
            mGraph->setFormatFunctX(nullptr);
            mGraph->setFormatFunctY(nullptr);
            mGraph->setYAxisMode(GraphView::eHidden);

            mTitle = tr("Phase Activity : %1").arg(mPhase->mName);
            GraphCurve curveActivity = densityCurve( mPhase->mActivity,
                                                     "Post Distrib All Chains",
                                                     color, Qt::SolidLine);
            curveActivity.mIsRectFromZero = true;
            auto brushColor = color;
            brushColor.setAlpha(30);

            const GraphCurve &curveActivityEnv = shapeCurve(mPhase->mActivityInf, mPhase->mActivitySup,
                                                     "Post Distrib Env All Chains",
                                                     color, Qt::CustomDashLine, brushColor);
            /* ------------------------------------------------------------
             *   Display envelope Uniform
             * ------------------------------------------------------------*/

            const GraphCurve &curveActivityUnifTheo = densityCurve(mPhase->mActivityUnifTheo,
                                                            "Post Distrib Unif Mean",
                                                            Qt::darkGray, Qt::SolidLine);

            mGraph->setInfo(QString("Significance Score ( %1 %) = %2").arg(stringForLocal(mPhase->mValueStack.at("Activity_Threshold").mValue), stringForLocal(mPhase->mValueStack.at("Significance Score").mValue, true)) );

            mGraph->setOverArrow(GraphView::eBothOverflow);

            mGraph->addCurve(curveActivityEnv);
            mGraph->addCurve(curveActivity);

            mGraph->addCurve(curveActivityUnifTheo);

            const type_data yMax = std:: max(map_max_value(mPhase->mActivitySup), map_max_value(mPhase->mActivityUnifTheo));

            mGraph->setRangeY(0., yMax);

            /* ------------------------------------------------------------
        *   Add zones outside study period
        * ------------------------------------------------------------*/

            const GraphZone zoneMin (-INFINITY, mSettings.getTminFormated());
            mGraph->addZone(zoneMin);

            const GraphZone zoneMax (mSettings.getTmaxFormated(), INFINITY);
            mGraph->addZone(zoneMax);

        } else
            mGraph->resetNothingMessage();

    } else if (typeGraph == ePostDistrib && mCurrentVariableList.contains(eDuration)) {
        mGraph->mLegendX = tr("Years");
        mGraph->mLegendY = "";
        mGraph->setFormatFunctX(nullptr);
        mGraph->setFormatFunctY(nullptr);
        mGraph->setYAxisMode(GraphView::eMinMaxHidden);
        mTitle = tr("Phase Duration : %1").arg(mPhase->mName);

        if (mPhase->mDuration.fullHisto().size() > 1) {
            const GraphCurve &curveDuration = densityCurve(mPhase->mDuration.fullHisto(), "Post Distrib All Chains", color);

            mGraph->setRangeX(0., ceil(curveDuration.mData.lastKey()));
            color.setAlpha(255);
            GraphCurve curveDurationHPD = HPDCurve(mPhase->mDuration.mHPD, "HPD All Chains", color);
            mGraph->setCanControlOpacity(true);
            mGraph->addCurve(curveDurationHPD);
            mGraph->setFormatFunctX(nullptr);
            mGraph->setFormatFunctY(nullptr);

            mGraph->addCurve(curveDuration);


            /* ------------------------------------
             *  Theta Credibility
             * ------------------------------------
             */
            GraphCurve curveCred = topLineSection(mPhase->mDuration.mCredibility,
                                                            "Credibility All Chains",
                                                            color);
            mGraph->addCurve(curveCred);

        } else
            mGraph->resetNothingMessage();


        if (!mPhase->mDuration.mChainsHistos.isEmpty())
            for (int i = 0; i < mChains.size(); ++i) {
                const GraphCurve &curveDuration = densityCurve(mPhase->mDuration.histoForChain(i),
                                                             "Post Distrib Chain " + QString::number(i),
                                                             Painting::chainColors.at(i), Qt::DotLine);

                mGraph->addCurve(curveDuration);
            }

    }

    /* -----------------second tab : history plot-------------------------------
     *  - Trace Begin i
     *  - Q1 Begin i
     *  - Q2 Begin i
     *  - Q3 Begin i
     *  - Trace End i
     *  - Q1 End i
     *  - Q2 End i
     *  - Q3 End i
     * ------------------------------------------------ */
    else if (typeGraph == eTrace && mCurrentVariableList.contains(eBeginEnd)) {
        mGraph->mLegendX = tr("Iterations");
        mGraph->setFormatFunctX(nullptr);
        mGraph->setFormatFunctY(nullptr);//DateUtils::convertToAppSettingsFormat);
        mGraph->setYAxisMode(GraphView::eMinMax);
        mTitle = tr("Phase : %1").arg(mPhase->mName);

        generateTraceCurves(mChains, &(mPhase->mAlpha), "Begin");
        generateTraceCurves(mChains, &(mPhase->mBeta), "End");
        mGraph->autoAdjustYScale(true);

    } else if (typeGraph == eTrace && mCurrentVariableList.contains(eDuration)) {
        mGraph->mLegendX = tr("Iterations");
        mGraph->setFormatFunctX(nullptr);
        mGraph->setFormatFunctY(nullptr);
        mGraph->setYAxisMode(GraphView::eMinMax);
        mTitle = tr("Phase Duration : %1").arg(mPhase->mName);

        generateTraceCurves(mChains, &(mPhase->mDuration), "Duration");
        mGraph->autoAdjustYScale(true);

    }
    /* ------------------------------------------------
     *  third tab : Nothing
     *  fourth tab : Nothing
     * ------------------------------------------------ */
    else {
       mTitle = tr("Phase : %1").arg(mPhase->mName);
       mGraph->resetNothingMessage();
    }

}

void GraphViewPhase::updateCurvesToShow(bool showAllChains, const QList<bool>& showChainList, const QVector<variable_t>& showVariableList)
{
    Q_ASSERT(mPhase);
    GraphViewResults::updateCurvesToShow(showAllChains, showChainList, showVariableList);

    /* --------------------first tab : posterior distrib----------------------------
     *
     *  Possible curves :
     *  - Post Distrib Begin-End All Chains
     *  - HPD Begin All Chains
     *  - HPD End All Chains
     *
     *  - Post Distrib Begin Chain i
     *  - Post Distrib End Chain i
     *
     *  -Duration
     *  -- Post Distrib All Chains
     *  -- HPD All Chains
     *  -- Credibility All Chains
     *  -- Post Distrib Chain i
     *
     *  - Tempo
     *  - Tempo All Chains
     *  - Tempo Error Env All Chain
     *
     *  - Activity
     *  -- All Chains
     *  -- Env All Chain
     *
     * ------------------------------------------------
     */
    if (mCurrentTypeGraph == ePostDistrib) {
        if (mShowVariableList.contains(eBeginEnd)) {
            const bool showCredibility = true;


            mGraph->setCurveVisible("Post Distrib Begin All Chains", mShowAllChains);
            mGraph->setCurveVisible("Post Distrib End All Chains", mShowAllChains);
            mGraph->setCurveVisible("HPD Begin All Chains", mShowAllChains);
            mGraph->setCurveVisible("HPD End All Chains", mShowAllChains);

            mGraph->setCurveVisible("Time Range", mShowAllChains && showCredibility);

            for (auto i=0; i<mShowChainList.size(); ++i) {
                mGraph->setCurveVisible("Post Distrib Begin Chain " + QString::number(i), mShowChainList.at(i));
                mGraph->setCurveVisible("Post Distrib End Chain " + QString::number(i), mShowChainList.at(i));
            }

            mGraph->setTipXLab("t");
            mGraph->setTipYLab("");

            mGraph->showInfos(false);
            mGraph->clearInfos();
            mGraph->autoAdjustYScale(true);

        }

        else if (mShowVariableList.contains(eDuration)) {
            const GraphCurve* duration = mGraph->getCurve("Post Distrib All Chains");

            if ( duration && !duration->mData.isEmpty()) {
                const bool showCredibility = true;
                mGraph->setCurveVisible("Post Distrib All Chains", mShowAllChains);
                mGraph->setCurveVisible("HPD All Chains", mShowAllChains);
                mGraph->setCurveVisible("Credibility All Chains", showCredibility && mShowAllChains);

                for (auto i=0; i<mShowChainList.size(); ++i)
                    mGraph->setCurveVisible("Post Distrib Chain " + QString::number(i), mShowChainList.at(i));

                mGraph->setTipXLab("t");
                mGraph->setTipYLab("");
                mGraph->autoAdjustYScale(true);
            }

        }
        else if (mShowVariableList.contains(eTempo)) {
            // With variable eTempo there is no choice of "chain", it must be "all chains"
            const GraphCurve* tempo = mGraph->getCurve("Post Distrib All Chains");

            if ( tempo && !tempo->mData.isEmpty()) {
                const bool showError = mShowVariableList.contains(eError);
                mGraph->setCurveVisible("Post Distrib All Chains", true);
                mGraph->setCurveVisible("Post Distrib Env All Chains", showError);

            }

        }
        else if (mShowVariableList.contains(eActivity)) {
            // With variable eActivity there is no choice of "chain", it must be "all chains"
            const GraphCurve* Activity = mGraph->getCurve("Post Distrib All Chains");

            if ( Activity && !Activity->mData.isEmpty()) {
                const bool showError = mShowVariableList.contains(eError);
                mGraph->setCurveVisible("Post Distrib All Chains", true);
                mGraph->setCurveVisible("Post Distrib Env All Chains", showError);

                // envelope Uniform
                const bool showActivityUnif = mShowVariableList.contains(eActivityUnif);
                mGraph->setCurveVisible("Post Distrib Unif Mean", showActivityUnif);

                mGraph->setTipXLab("t");
                mGraph->setTipYLab("A");

                mGraph->autoAdjustYScale(true);
            }

        }
    }
    /* ---------------- second tab : history plot--------------------------------
     *  - Begin Trace i
     *  - Begin Q1 i
     *  - Begin Q2 i
     *  - Begin Q3 i
     *  - End Trace i
     *  - End Q1 i
     *  - End Q2 i
     *  - End Q3 i
     *
     *  - Duration Trace i
     *  - Duration Q1 i
     *  - Duration Q2 i
     *  - Duration Q3 i
     * ------------------------------------------------ */
    else if (mCurrentTypeGraph == eTrace) {
        if (mShowVariableList.contains(eBeginEnd)) {

            for (int i = 0; i<mShowChainList.size(); ++i) {
                mGraph->setCurveVisible("Begin Trace " + QString::number(i), mShowChainList.at(i));
                mGraph->setCurveVisible("Begin Q1 " + QString::number(i), mShowChainList.at(i));
                mGraph->setCurveVisible("Begin Q2 " + QString::number(i), mShowChainList.at(i));
                mGraph->setCurveVisible("Begin Q3 " + QString::number(i), mShowChainList.at(i));

                mGraph->setCurveVisible("End Trace " + QString::number(i), mShowChainList.at(i));
                mGraph->setCurveVisible("End Q1 " + QString::number(i), mShowChainList.at(i));
                mGraph->setCurveVisible("End Q2 " + QString::number(i), mShowChainList.at(i));
                mGraph->setCurveVisible("End Q3 " + QString::number(i), mShowChainList.at(i));
            }

            mGraph->setTipXLab(tr("Iteration"));
            mGraph->setTipYLab("t");
            mGraph->setYAxisMode(GraphView::eMinMaxHidden);
            mGraph->showInfos(false);
            mGraph->autoAdjustYScale(true);

        } else if (mShowVariableList.contains(eDuration)) {

            for (int i = 0; i<mShowChainList.size(); ++i) {
                mGraph->setCurveVisible("Duration Trace " + QString::number(i), mShowChainList.at(i));
                mGraph->setCurveVisible("Duration Q1 " + QString::number(i), mShowChainList.at(i));
                mGraph->setCurveVisible("Duration Q2 " + QString::number(i), mShowChainList.at(i));
                mGraph->setCurveVisible("Duration Q3 " + QString::number(i), mShowChainList.at(i));
            }

            mGraph->setTipXLab(tr("Iteration"));
            mGraph->setTipYLab("t");
            mGraph->setYAxisMode(GraphView::eMinMaxHidden);
            mGraph->showInfos(false);
            mGraph->autoAdjustYScale(true);
        }
    }
    repaint();
}
