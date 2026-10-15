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

#include "GraphViewS02Vg.h"
#include "GraphView.h"
#include "Painting.h"
#include "ModelUtilities.h"
#include "QtUtilities.h"

#include <QtWidgets>

GraphViewS02Vg::GraphViewS02Vg(QWidget* parent):
    GraphViewResults(parent)
{
    setMainColor(Painting::borderDark);
    mGraph->setBackgroundColor(QColor(210, 210, 210));
}

GraphViewS02Vg::~GraphViewS02Vg()
{
}

void GraphViewS02Vg::generateCurves(const graph_t typeGraph, const QList<variable_t> &variableList)
{
    auto model = getModel_ptr();
    GraphViewResults::generateCurves(typeGraph, variableList);
    
    mGraph->removeAllCurves();

    mGraph->clearInfos();
    mGraph->resetNothingMessage();
    mGraph->setOverArrow(GraphView::OverflowDataArrowMode::eNone);
    mGraph->reserveCurves(6);

    QPen defaultPen;
    defaultPen.setWidthF(1);
    defaultPen.setStyle(Qt::SolidLine);

    const QString resultsHTML = ModelUtilities::S02VgResultsHTML(model);
    setNumericalResults(resultsHTML);

#ifdef KOMLAN
    QColor color = Qt::blue;
    // ------------------------------------------------
    //  First tab : Posterior distrib
    // ------------------------------------------------
    if (typeGraph == ePostDistrib) {
        mGraph->mLegendX = "S02Vg";
        mGraph->setFormatFunctX(nullptr);
        mGraph->setFormatFunctY(nullptr);
        mGraph->setBackgroundColor(QColor(230, 230, 230));
        mGraph->setOverArrow(GraphView::OverflowDataArrowMode::eBothOverflow);

        //mGraph->setXAxisSupport(AxisTool::AxisSupport::eAllTip);
        //mGraph->setYAxisSupport(AxisTool::AxisSupport::eAllways_Positive);

        mGraph->autoAdjustYScale(true);

        mGraph->setXAxisMode(GraphView::OverflowDataArrowMode::eAllTicks);
        mGraph->setYAxisMode(GraphView::OverflowDataArrowMode::eHidden);



        mTitle = tr("S02Vg");

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

        for (size_t i=0; i<mChains.size(); ++i)  {
            const GraphCurve &curvePostDistribChain = densityCurve(model->mS02Vg.KDEForChain(i),
                                                                   "Post Distrib Chain " + QString::number(i),
                                                                   Painting::chainColors.at(i),
                                                                   Qt::SolidLine,
                                                                   Qt::NoBrush);
            mGraph->add_curve(curvePostDistribChain);
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
        mGraph->setTipYLab("Log10 S02Vg");
        mGraph->setFormatFunctX(nullptr);
        mTitle = tr("S02Vg Trace");

        if (model->mS02Vg.mSamplerProposal != MHVariable::eFixe)
            generateLogTraceCurves(mChains, &(model->mS02Vg));
        else

            mGraph->resetNothingMessage();
    }
    // -------------------------------------------------
    //  Acceptance rate
    // -------------------------------------------------
    else if (typeGraph == eAccept) {
        mGraph->mLegendX = "Iterations";
        mGraph->setTipYLab("Rate");
        mGraph->setFormatFunctX(nullptr);
        mTitle = tr("S02Vg Acceptation");
#ifdef KOMLAN
        if (model->mS02Vg.mSamplerProposal != MHVariable::eFixe)
            generateAcceptCurves(mChains, &(model->mS02Vg));
        else
#endif
            mGraph->resetNothingMessage();
    }

    // -------------------------------------------------
    //  Autocorrelation
    // -------------------------------------------------
    else if (typeGraph == eCorrel) {
        mGraph->mLegendX = "";
        mGraph->setFormatFunctX(nullptr);
        mTitle = tr("S02Vg Autocorrelation");

        if (model->mS02Vg.mSamplerProposal != MHVariable::eFixe) {
            generateCorrelCurves(mChains, &(model->mS02Vg));
            mGraph->setXScaleDivision(10, 10);
        }
        else
            mGraph->resetNothingMessage();

    } else  {
        mTitle = tr("S02Vg");
        mGraph->resetNothingMessage();
    }
#endif
        mTitle = tr("S02Vg");
        mGraph->resetNothingMessage();

}

void GraphViewS02Vg::updateCurvesToShow(bool showAllChains, const QList<bool>& showChainList, const QList<variable_t> &variableList)
{
#ifdef KOMLAN
    GraphViewResults::updateCurvesToShow(showAllChains, showChainList, variableList);

    if (mCurrentTypeGraph == ePostDistrib)  {
        mGraph->setTipYLab("");

        QStringList curvesToShow;

        if (mShowAllChains) {
            curvesToShow << "Post Distrib All Chains" << "HPD All Chains";
            if (mShowList.contains(eCredibility))
                curvesToShow << "Credibility All Chains";

        }

        // Ajouter les chaînes individuelles
        for (int i = 0; i < mShowChainList.size(); ++i) {
            if (mShowChainList.at(i))
                curvesToShow << QString("Post Distrib Chain %1").arg(i);

        }

        mGraph->setCurveVisible(curvesToShow, true);


        mGraph->setTipXLab(tr("S02Vg"));

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

        QStringList curvesToShow;
        for (int j = 0; j < mShowChainList.size(); ++j) {
            if (mShowChainList.at(j)) {
                curvesToShow << QString("Trace %1").arg(j);
                curvesToShow << QString("Q1 %1").arg(j);
                curvesToShow << QString("Q2 %1").arg(j);
                curvesToShow << QString("Q3 %1").arg(j);
            }
        }
        mGraph->setCurveVisible(curvesToShow, true);

        mGraph->setTipXLab(tr("Iteration"));
        mGraph->setTipYLab(tr("Log10(S02Vg)"));

        mGraph->setXAxisSupport(AxisTool::AxisSupport::eAllways_Positive);
        mGraph->setYAxisSupport(AxisTool::AxisSupport::eMin_Max);
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

        QStringList curvesToShow;
        curvesToShow << "Accept Target";
        for (int j = 0; j < mShowChainList.size(); ++j) {
            if (mShowChainList.at(j)) {
                curvesToShow << QString("Accept %1").arg(j);
            }
        }
        mGraph->setCurveVisible(curvesToShow, true);

        mGraph->setTipXLab(tr("Iteration"));
        mGraph->setTipYLab(tr("Rate"));

        mGraph->setXAxisSupport(AxisTool::AxisSupport::eAllways_Positive);
        mGraph->setYAxisSupport(AxisTool::AxisSupport::eAllways_Positive);

        mGraph->setYAxisMode(GraphView::eMinMax );
        mGraph->showInfos(false);
        mGraph->clearInfos();
        mGraph->autoAdjustYScale(false);
        mGraph->setRangeY(0, 100);
    }


      /* ----------------------fourth tab : Autocorrelation--------------------------
       *  Possible curves :
       *  - Correl i
       *  - Correl Limit Lower i
       *  - Correl Limit Upper i
       * ------------------------------------------------   */
      else if (mCurrentTypeGraph == eCorrel) {

          QStringList curvesToShow;
          for (int j = 0; j < mShowChainList.size(); ++j) {
              if (mShowChainList.at(j)) {
                  curvesToShow << QString("Correl %1").arg(j);
                  curvesToShow << QString("Correl Limit Lower %1").arg(j);
                  curvesToShow << QString("Correl Limit Upper %1").arg(j);
              }
          }
          mGraph->setCurveVisible(curvesToShow, true);

          mGraph->setTipXLab("h");
          mGraph->setTipYLab(tr("Value"));

          mGraph->setXAxisSupport(AxisTool::AxisSupport::eAllways_Positive);
          mGraph->setYAxisSupport(AxisTool::AxisSupport::eAllways_Positive);

          mGraph->setYAxisMode(GraphView::eMinMax);
          mGraph->showInfos(false);
          mGraph->clearInfos();
          mGraph->autoAdjustYScale(false); // do  repaintGraph()
          mGraph->setRangeY(-1, 1);
      }

      repaint();
#endif
}
