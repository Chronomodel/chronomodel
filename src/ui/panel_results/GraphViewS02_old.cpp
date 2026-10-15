/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
--------------------------------------------------------------------- */

#include "GraphViewS02_old.h"
#include "GraphView.h"
#include "Painting.h"
#include "ModelUtilities.h"
#include "QtUtilities.h"

#include <QtWidgets>

GraphViewS02::GraphViewS02(QWidget *parent):GraphViewResults(parent)
{
    setMainColor(Painting::borderDark);
    mGraph->setBackgroundColor(QColor(210, 210, 210));
}

GraphViewS02::~GraphViewS02()
{
}
/*
void GraphViewS02::paintEvent(QPaintEvent* e)
{
    GraphViewResults::paintEvent(e);
}

void GraphViewS02::resizeEvent(QResizeEvent* )
{
    updateLayout();
}
*/
void GraphViewS02::generateCurves(const graph_t typeGraph, const QList<variable_t>& variableList)
{
    auto model = getModel_ptr();
    GraphViewResults::generateCurves(typeGraph, variableList);
    
    mGraph->removeAllCurves();
    mGraph->remove_all_zones();
    mGraph->clearInfos();
    mGraph->resetNothingMessage();
    mGraph->setOverArrow(GraphView::eNone);
    mGraph->reserveCurves(6);

    QPen defaultPen;
    defaultPen.setWidthF(1);
    defaultPen.setStyle(Qt::SolidLine);

    QColor color = Qt::blue;

    QString resultsHTML = ModelUtilities::S02ResultsHTML(model);
    setNumericalResults(resultsHTML);
    // ------------------------------------------------
    //  First tab : Posterior distrib
    // ------------------------------------------------
    if (typeGraph == ePostDistrib) {
        mGraph->mLegendX = "sqrt";
        mGraph->setFormatFunctX(nullptr);
        mGraph->setFormatFunctY(nullptr);
        mGraph->setBackgroundColor(QColor(230, 230, 230));
        mGraph->setOverArrow(GraphView::eBothOverflow);

        mGraph->setXAxisSupport(AxisTool::AxisSupport::eAllTip);
        mGraph->setYAxisSupport(AxisTool::AxisSupport::eAllways_Positive);

        mGraph->autoAdjustYScale(true);

        mGraph->setXAxisMode(GraphView::eAllTicks);
        mGraph->setYAxisMode(GraphView::eHidden);

        mTitle = tr("Variance Param.");

        // ------------------------------------
        //  Post distrib All Chains
        // ------------------------------------

        const GraphCurve &curvePostDistrib = densityCurve(model->mS02Vg.fullHisto(),
                                                          "Post Distrib All Chains",
                                                          color);
        mGraph->add_curve(curvePostDistrib);

        // ------------------------------------
        //  HPD All Chains
        // ------------------------------------
        const GraphCurve &curveHPD = HPDCurve(model->mS02Vg.mFormatedHPD, "HPD All Chains", color);
        mGraph->add_curve(curveHPD);

        // ------------------------------------
        //  Post Distrib Chain i
        // ------------------------------------
        if (!model->mS02Vg.mChainsHistos.empty()) {
            for (size_t i=0; i<mChains.size(); ++i)  {
                const GraphCurve& curvePostDistribChain = densityCurve(model->mS02Vg.histoForChain(i),
                                                                       "Post Distrib Chain " + QString::number(i),
                                                                       Painting::chainColors.at(i),
                                                                       Qt::SolidLine,
                                                                       Qt::NoBrush);
                mGraph->add_curve(curvePostDistribChain);
            }
        }

        // ------------------------------------
        //  Theta Credibility
        // ------------------------------------
        const GraphCurve &curveCred = topLineSection(model->mS02Vg.mFormatedCredibility,
                                                     "Credibility All Chains",
                                                     color);
        mGraph->add_curve(curveCred);
        mGraph->autoAdjustYScale(true);

    }
    // -------------------------------------------------
    //  History plots
    // -------------------------------------------------
    else if (typeGraph == eTrace) {
        mGraph->mLegendX = "Iterations";
        mGraph->setTipYLab("S02");
        mGraph->setFormatFunctX(nullptr);
        mTitle = tr("Variance Param. Trace");
        if (model->mS02Vg.mSamplerProposal != MHVariable::eFixe)
            generateTraceCurves(mChains, &(model->mS02Vg));
        else
            mGraph->resetNothingMessage();
    }
    // -------------------------------------------------
    //  Acceptance rate
    // -------------------------------------------------
    else if (typeGraph == eAccept) {
       /* mGraph->mLegendX = "Iterations";
        mGraph->setTipYLab("Rate");
        mGraph->setFormatFunctX(nullptr);
        mTitle = tr("Variance Param. Acceptation");
        if (model->mS02Vg.mSamplerProposal != MHVariable::eFixe)
            generateAcceptCurves(mChains, &(model->mS02Vg));
         else */
            mGraph->resetNothingMessage();

        //mGraph->repaint();
    }

    // -------------------------------------------------
    //  Autocorrelation
    // -------------------------------------------------
    else if (typeGraph == eCorrel) {
       /* mGraph->mLegendX = "";
        mGraph->setFormatFunctX(nullptr);
        mTitle = tr("Variance Param. Autocorrelation");
        if (model->mS02Vg.mSamplerProposal != MHVariable::eFixe) {
            generateCorrelCurves(mChains, &(model->mS02Vg));
            mGraph->setXScaleDivision(10, 10);
        }
        else
            mGraph->resetNothingMessage();

    } else  { */
        mTitle = tr("Variance Param.");
        mGraph->resetNothingMessage();
    }
}

void GraphViewS02::updateCurvesToShow(bool showAllChains, const QList<bool>& showChainList, const QList<variable_t>& variableList)
{
	GraphViewResults::updateCurvesToShow(showAllChains, showChainList, variableList);

    if (mCurrentTypeGraph == ePostDistrib)  {
        mGraph->setTipYLab("");
        
        mGraph->setCurveVisible("Post Distrib All Chains", mShowAllChains);
        mGraph->setCurveVisible("HPD All Chains", mShowAllChains);
        mGraph->setCurveVisible("Credibility All Chains", mShowAllChains && mShowVariableList.contains(eCredibility));

        for (qsizetype i = 0; i<mShowChainList.size(); ++i) {
            mGraph->setCurveVisible("Post Distrib Chain " + QString::number(i), mShowChainList.at(i));
        }
        
        mGraph->setTipXLab("sqrt Variance Param.");
        mGraph->setYAxisMode(GraphView::eHidden);
        mGraph->showInfos(false);
        mGraph->clearInfos();
    }
    /* ------------------Second tab : History plots.------------------------------
     *  Possible Curves (could be for theta or sigma):
     *  - Trace i
     *  - Q1 i
     *  - Q2 i
     *  - Q3 i
     * ------------------------------------------------
     */
    else if (mCurrentTypeGraph == eTrace)  {
        for (qsizetype i = 0; i<mShowChainList.size(); ++i) {
            mGraph->setCurveVisible("Trace " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Q1 " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Q2 " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Q3 " + QString::number(i), mShowChainList.at(i));
        }

        mGraph->setTipXLab(tr("Iteration"));
        mGraph->setTipYLab("sqrt Variance Param.");

        mGraph->setYAxisMode(GraphView::eMinMaxHidden);
        mGraph->showInfos(false);
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

      /* ----------------------fourth tab : Autocorrelation--------------------------
       *  Possible curves :
       *  - Correl i
       *  - Correl Limit Lower i
       *  - Correl Limit Upper i
       * ------------------------------------------------   */
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
          mGraph->autoAdjustYScale(false); // do  repaintGraph()
          mGraph->setRangeY(-1, 1);
      }

      repaint();
}
