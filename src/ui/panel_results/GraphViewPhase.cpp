/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2025

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

// Constructor / Destructor

GraphViewPhase::GraphViewPhase(QWidget* parent):
    GraphViewResults(parent),
    mPhase(nullptr)
{
    setMainColor(Painting::borderDark);
    mGraph->setBackgroundColor(QColor(210, 210, 210));
}

GraphViewPhase::~GraphViewPhase()
{
}

void GraphViewPhase::setPhase(std::shared_ptr<Phase> phase)
{
    Q_ASSERT(phase);
    mPhase = phase;
    setItemColor(mPhase->mColor);
}

void GraphViewPhase::generateCurves(const graph_t typeGraph, const QList<variable_t>& variableList)
{
    GraphViewResults::generateCurves(typeGraph, variableList);

    graph_reset();

    mTitle = tr("Phase : %1").arg(mPhase->getQStringName());

    QPen defaultPen;
    defaultPen.setWidthF(1.);
    defaultPen.setStyle(Qt::SolidLine);

    QColor color = mPhase->mColor;

    QString resultsHTML = tr("Nothing to Display");
    if (mCurrentVariableList.contains(eBeginEnd)) {
        resultsHTML = ModelUtilities::phaseResultsHTML(mPhase);

    } else if (mCurrentVariableList.contains(eTempo)) {
        resultsHTML = ModelUtilities::tempoResultsHTML(mPhase);

    } else if (mCurrentVariableList.contains(eDuration)) {
        resultsHTML = ModelUtilities::durationResultsHTML(mPhase);

    } else if (mCurrentVariableList.contains(eActivity)) {
        resultsHTML = ModelUtilities::activityResultsHTML(mPhase);
    }

    setNumericalResults(resultsHTML);

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
    if (typeGraph == ePostDistrib) {
        if (mCurrentVariableList.contains(eBeginEnd)) {
            graph_density();
            mGraph->reserveCurves(5 + 2*mChains.size());

            std::map<double, double> &alpha = mPhase->mAlpha.mFormatedHisto;
            std::map<double, double> &beta = mPhase->mBeta.mFormatedHisto;

            std::map<double, double> &alphaHPD = mPhase->mAlpha.mFormatedHPD;
            std::map<double, double> &betaHPD = mPhase->mBeta.mFormatedHPD;
            /*
             * Detection of one Bound used as boundary != is xor
             * If there is two Bound, the both are egal to 1, thus nothing to do
             */
            const bool alphaIsBound = (alpha.size()==1);
            const bool betaIsBound = (beta.size()==1);

            if (alphaIsBound && !betaIsBound) {
                const double normPdf = map_max(beta)->second;
                alpha[alpha.begin()->first] =  normPdf;
                alphaHPD[alphaHPD.begin()->first] = normPdf;

            } else if (betaIsBound && !alphaIsBound) {
                const double normPdf = map_max(alpha)->second;
                beta[beta.begin()->first] = normPdf;
                betaHPD[betaHPD.begin()->first] = normPdf;

            } else if (alphaIsBound && betaIsBound) {
                alpha[alpha.begin()->first] =  1.;
                alphaHPD[alphaHPD.begin()->first] = 1.;

                beta[beta.begin()->first] = 1.;
                betaHPD[betaHPD.begin()->first] = 1.;
            }

            const GraphCurve &curveBegin = densityCurve(alpha, "Post Distrib Begin All Chains", color, Qt::DotLine);
            const QColor colorEnd = mPhase->mColor.darker(170);

            const GraphCurve &curveEnd = densityCurve(beta, "Post Distrib End All Chains", colorEnd, Qt::DashLine);
            color.setAlpha(255); // set mBrush to fill
            const GraphCurve &curveBeginHPD = HPDCurve(alphaHPD, "HPD Begin All Chains", color);

            const GraphCurve &curveEndHPD = HPDCurve(betaHPD, "HPD End All Chains", colorEnd);

            mGraph->add_curve(curveBegin);
            mGraph->add_curve(curveEnd);

            mGraph->add_curve(curveBeginHPD);
            mGraph->add_curve(curveEndHPD);

            const GraphCurve &curveTimeRange = topLineSection(mPhase->getFormatedTimeRange(), "Time Range", color);
            mGraph->add_curve(curveTimeRange);

            if (!mPhase->mAlpha.mChainsHistos.empty())
                for (size_t i = 0; i<mChains.size(); ++i) {
                    std::map<double, double> &alpha_i = mPhase->mAlpha.mChainsHistos[i];
                    std::map<double, double> &beta_i = mPhase->mBeta.mChainsHistos[i];

                    if (alphaIsBound && !betaIsBound) {
                        alpha_i[alpha_i.begin()->first] =  map_max(beta_i)->second;

                    } else if (betaIsBound && !alphaIsBound) {
                        beta_i[beta_i.begin()->first] = map_max(alpha_i)->second;
                    }

                    const GraphCurve &curveBegin = densityCurve(alpha_i,
                                                                "Post Distrib Begin Chain " + QString::number(i),
                                                                Painting::chainColors.at(i), Qt::DotLine);

                    const GraphCurve &curveEnd = densityCurve(beta_i,
                                                              "Post Distrib End Chain " + QString::number(i),
                                                              Painting::chainColors.at(i).darker(170), Qt::DashLine);
                    mGraph->add_curve(curveBegin);
                    mGraph->add_curve(curveEnd);
                }


        } else if (mCurrentVariableList.contains(eTempo)) {

            if (!mPhase->mTempo.empty()) {
                graph_density();
                mGraph->reserveCurves(2);

                mGraph->mLegendX = DateUtils::getAppSettingsFormatStr();
                mGraph->mLegendY = "Events";

                mGraph->setTipXLab("t");
                mGraph->setTipYLab("n");
                //mGraph->setYAxisMode(GraphView::eAllTicks);

                mTitle = tr("Phase Tempo : %1").arg(mPhase->getQStringName());

                GraphCurve curveTempo = densityCurve(mPhase->mTempo,
                                                     "Post Distrib All Chains",
                                                     color.darker(), Qt::SolidLine);
                curveTempo.mIsRectFromZero = false;

                auto brushColor = color;
                brushColor.setAlpha(30);
                const GraphCurve &curveTempoEnv = shapeCurve(mPhase->mTempoInf, mPhase->mTempoSup,
                                                             "Post Distrib Env All Chains",
                                                             color, Qt::CustomDashLine, brushColor);

                mGraph->add_curve(curveTempoEnv);
                mGraph->add_curve(curveTempo);

            }


        } else if (mCurrentVariableList.contains(eActivity)) {

            if (!mPhase->mActivity.empty()) {
                graph_density();

                mGraph->setTipXLab("t");
                mGraph->setTipYLab("A");

                mGraph->reserveCurves(3);

                mTitle = tr("Phase Activity : %1").arg(mPhase->getQStringName());
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

                GraphCurve curveActivityUnifTheo = densityCurve(mPhase->mActivityUnifTheo,
                                                                       "Post Distrib Unif Mean",
                                                                       Qt::darkGray, Qt::SolidLine);

                curveActivityUnifTheo.mIsRectFromZero = false;
                mGraph->add_curve(curveActivityEnv);
                mGraph->add_curve(curveActivity);

                mGraph->add_curve(curveActivityUnifTheo);


            }

        } else if (mCurrentVariableList.contains(eDuration)) {
            graph_density();
            mGraph->remove_all_zones();
            mGraph->reserveCurves(3 + mChains.size());
            mGraph->mLegendX = tr("Years");

            mGraph->setXAxisSupport(AxisTool::AxisSupport::eAllways_Positive);
            mGraph->setYAxisSupport(AxisTool::AxisSupport::eAllways_Positive);

            mGraph->setYAxisMode(GraphView::eMinMaxHidden);
            mTitle = tr("Phase Duration : %1").arg(mPhase->getQStringName());

            if (mPhase->mDuration.fullHisto().size() > 0) {
                const GraphCurve &curveDuration = densityCurve(mPhase->mDuration.fullHisto(), "Post Distrib All Chains", color);
                mGraph->add_curve(curveDuration);

               // mGraph->setRangeX(0., ceil(curveDuration.mData.lastKey()));
                color.setAlpha(255);
                const GraphCurve &curveDurationHPD = HPDCurve(mPhase->mDuration.mFormatedHPD, "HPD All Chains", color);
                mGraph->setCanControlOpacity(true);
                mGraph->add_curve(curveDurationHPD);

                /* ------------------------------------
                 *  Theta Credibility
                 * ------------------------------------
                */
                const GraphCurve &curveCred = topLineSection(mPhase->mDuration.mFormatedCredibility,
                                                      "Credibility All Chains",
                                                      color);
                mGraph->add_curve(curveCred);

            }


            if (!mPhase->mDuration.mChainsHistos.empty())
                for (size_t i = 0; i < mChains.size(); ++i) {
                    const GraphCurve &curveDuration = densityCurve(mPhase->mDuration.histoForChain(i),
                                                                   "Post Distrib Chain " + QString::number(i),
                                                                   Painting::chainColors.at(i), Qt::DotLine);

                    mGraph->add_curve(curveDuration);
                }

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
    else if (typeGraph == eTrace) {
        if (mCurrentVariableList.contains(eBeginEnd)) {
            graph_trace();
            mGraph->reserveCurves(2);

            generateTraceCurves(mChains, &(mPhase->mAlpha), "Begin");
            generateTraceCurves(mChains, &(mPhase->mBeta), "End");
            mGraph->autoAdjustYScale(true);

        } else if (mCurrentVariableList.contains(eDuration)) {
            graph_trace();
            mGraph->reserveCurves(1);
            mTitle = tr("Phase Duration : %1").arg(mPhase->getQStringName());

            generateTraceCurves(mChains, &(mPhase->mDuration), "Duration");
            mGraph->autoAdjustYScale(true);
        }
    }
    /* ------------------------------------------------
     *  Third tab : Nothing
     *  Fourth tab : Nothing
     * ------------------------------------------------ */
    else {
       mTitle = tr("Phase : %1").arg(mPhase->getQStringName());
       mGraph->resetNothingMessage();
    }

}

void GraphViewPhase::updateCurvesToShow(bool showAllChains, const QList<bool>& showChainList, const QList<variable_t>& showVariableList)
{
    Q_ASSERT(mPhase);
    GraphViewResults::updateCurvesToShow(showAllChains, showChainList, showVariableList);

    /* --------------------First tab : Posterior Distrib----------------------------
     *
     *  Possible curves :
     *  - Begin-End
     *  -- Post Distrib Begin-End All Chains
     *  -- HPD Begin All Chains
     *  -- HPD End All Chains
     *
     *  -- Post Distrib Begin Chain i
     *  -- Post Distrib End Chain i
     *
     *  -Duration
     *  -- Post Distrib All Chains
     *  -- HPD All Chains
     *  -- Credibility All Chains
     *  -- Post Distrib Chain i
     *
     *  - Tempo
     *  -- Tempo All Chains
     *  -- Tempo Error Env All Chain
     *
     *  - Activity
     *  -- All Chains
     *  -- Env All Chain
     *
     * ------------------------------------------------*/
    if (mCurrentTypeGraph == ePostDistrib) {

        if (mCurrentVariableList.contains(eBeginEnd)) {
            const bool showCredibility = mShowVariableList.contains(eCredibility);

            mGraph->setCurveVisible("Post Distrib Begin All Chains", mShowAllChains);
            mGraph->setCurveVisible("Post Distrib End All Chains", mShowAllChains);
            mGraph->setCurveVisible("HPD Begin All Chains", mShowAllChains);
            mGraph->setCurveVisible("HPD End All Chains", mShowAllChains);

            mGraph->setCurveVisible("Time Range", mShowAllChains && showCredibility);

            for (auto i=0; i<mShowChainList.size(); ++i) {
                mGraph->setCurveVisible("Post Distrib Begin Chain " + QString::number(i), mShowChainList.at(i));
                mGraph->setCurveVisible("Post Distrib End Chain " + QString::number(i), mShowChainList.at(i));
            }

        }

        else if (mCurrentVariableList.contains(eDuration)) {
            const GraphCurve* duration = mGraph->getCurve("Post Distrib All Chains");

            if ( duration && !duration->mData.isEmpty()) {
                const bool showCredibility = mShowVariableList.contains(eCredibility);
                mGraph->setCurveVisible("Post Distrib All Chains", mShowAllChains);
                mGraph->setCurveVisible("HPD All Chains", mShowAllChains);
                mGraph->setCurveVisible("Credibility All Chains", showCredibility && mShowAllChains);

                for (auto i=0; i<mShowChainList.size(); ++i)
                    mGraph->setCurveVisible("Post Distrib Chain " + QString::number(i), mShowChainList.at(i));

            }

        }
        else if (mCurrentVariableList.contains(eTempo)) {
            // With variable eTempo there is no choice of "chain", it must be "all chains"
            const GraphCurve* tempo = mGraph->getCurve("Post Distrib All Chains");

            if ( tempo && !tempo->mData.isEmpty()) {
                const bool showError = mShowVariableList.contains(eError);
                mGraph->setCurveVisible("Post Distrib All Chains", true);
                mGraph->setCurveVisible("Post Distrib Env All Chains", showError);
            }

        }
        else if (mCurrentVariableList.contains(eActivity)) {
            // With variable eActivity there is no choice of "chain", it must be "all chains"
            const GraphCurve* Activity = mGraph->getCurve("Post Distrib All Chains");

            if ( Activity && !Activity->mData.isEmpty()) {

                const bool showError = mShowVariableList.contains(eError);
                mGraph->setCurveVisible("Post Distrib All Chains", true);
                mGraph->setCurveVisible("Post Distrib Env All Chains", showError);

                // This text is displayed by ResultsView using the mInfos property of graph
                if (showError
                    && mPhase->mEvents.size()>1
                    && mPhase->mValueStack.contains("Activity_Threshold") && mPhase->mValueStack.contains("Activity_Significance_Score") ) {
                    QString txt = QString("h = %1  \u2192  Significance Score (%2 %) = %3").arg(
                        stringForLocal(mPhase->mValueStack.at("Activity_h")),
                        stringForLocal(mPhase->mValueStack.at("Activity_Threshold")),
                        stringForLocal(mPhase->mValueStack.at("Activity_Significance_Score"), true));
                    mGraph->setInfo(txt );
                } else
                    mGraph->clearInfos();

                // Activity Uniform
                const bool showActivityUnif = mShowVariableList.contains(eActivityUnif);
                mGraph->setCurveVisible("Post Distrib Unif Mean", showActivityUnif);

                /*
                type_data yMax = map_max(mPhase->mActivity)->second;
                if (showError) {
                    yMax = std:: max(yMax, map_max(mPhase->mActivitySup)->second);
                }
                if (showActivityUnif) {
                    yMax = std:: max(yMax, map_max(mPhase->mActivityUnifTheo)->second);
                }
                mGraph->setRangeY(0., yMax);
                 */
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
        if (mCurrentVariableList.contains(eBeginEnd)) {

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

        } else if (mCurrentVariableList.contains(eDuration)) {

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
