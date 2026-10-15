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

#include "GraphViewLambda.h"
#include "GraphView.h"
#include "Painting.h"
#include "ModelUtilities.h"
#include "QtUtilities.h"

#include <QtWidgets>

GraphViewLambda::GraphViewLambda(QWidget* parent):
    GraphViewResults(parent)
{
    setMainColor(Painting::borderDark);
    mGraph->setBackgroundColor(QColor(210, 210, 210));
}

GraphViewLambda::~GraphViewLambda()
{
}

void GraphViewLambda::generateCurves(const graph_t typeGraph, const QList<variable_t> &showList)
{
    updateCurves(typeGraph, showList, mShowAllChains, mShowChainList);
    return;

    auto model = getModel_ptr();
    GraphViewResults::generateCurves(typeGraph, showList);
    
    mGraph->removeAllCurves();

    mGraph->clearInfos();
    mGraph->resetNothingMessage();
    mGraph->setOverArrow(GraphView::OverflowDataArrowMode::eNone);
    mGraph->reserveCurves(6);

    QPen defaultPen;
    defaultPen.setWidthF(1);
    defaultPen.setStyle(Qt::SolidLine);

    QColor color = Qt::blue;

    const QString resultsHTML = ModelUtilities::lambdaResultsHTML(model);
    setNumericalResults(resultsHTML);

    // ------------------------------------------------
    //  First tab : Posterior distrib
    // ------------------------------------------------
    if (typeGraph == ePostDistrib) {
        mGraph->mLegendX = "Log10";
        mGraph->setFormatFunctX(nullptr);
        mGraph->setFormatFunctY(nullptr);
        mGraph->setBackgroundColor(QColor(230, 230, 230));
        mGraph->setOverArrow(GraphView::OverflowDataArrowMode::eBothOverflow);

        //mGraph->setXAxisSupport(AxisTool::AxisSupport::eAllTip);
        //mGraph->setYAxisSupport(AxisTool::AxisSupport::eAllways_Positive);

        mGraph->autoAdjustYScale(true);

        mGraph->setXAxisMode(GraphView::AxisMode::eAllTicks);
        mGraph->setYAxisMode(GraphView::AxisMode::eHidden);


        mTitle = tr("Smoothing");

        // ------------------------------------
        //  Post distrib All Chains
        // ------------------------------------

        const GraphCurve &curvePostDistrib = densityCurve(model->mLambdaSpline.mFormatedKDE,
                                                          "Post Distrib All Chains",
                                                          color);

        mGraph->add_curve(curvePostDistrib);

        // ------------------------------------
        //  HPD All Chains
        // ------------------------------------
        const GraphCurve &curveHPD = HPDCurve(model->mLambdaSpline.mFormatedHPD, "HPD All Chains", color);
        mGraph->add_curve(curveHPD);

        // ------------------------------------
        //  Post Distrib Chain i
        // ------------------------------------

        for (size_t i=0; i<mChains.size(); ++i)  {
            const GraphCurve &curvePostDistribChain = densityCurve(model->mLambdaSpline.KDEForChain(i),
                                                                   "Post Distrib Chain " + QString::number(i),
                                                                   Painting::chainColors.at(i),
                                                                   Qt::SolidLine,
                                                                   Qt::NoBrush);
            mGraph->add_curve(curvePostDistribChain);
        }

        
        // ------------------------------------
        //  Theta Credibility
        // ------------------------------------
        const GraphCurve &curveCred = topLineSection(model->mLambdaSpline.mFormatedCredibility,
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
        mGraph->setTipYLab("Smoothing");
        mGraph->setFormatFunctX(nullptr);
        mTitle = tr("Smoothing Trace");
        if (model->mLambdaSpline.mSamplerProposal != SamplerProposal::eFixe)
            generateTraceCurves(mChains, &(model->mLambdaSpline));
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
        mTitle = tr("Smoothing Acceptation");
        if (model->mLambdaSpline.mSamplerProposal != SamplerProposal::eFixe)
            generateAcceptCurves(mChains, &(model->mLambdaSpline));
        else
            mGraph->resetNothingMessage();
    }

    // -------------------------------------------------
    //  Autocorrelation
    // -------------------------------------------------
    else if (typeGraph == eCorrel) {
        mGraph->mLegendX = "";
        mGraph->setFormatFunctX(nullptr);
        mTitle = tr("Smoothing Autocorrelation");
        if (model->mLambdaSpline.mSamplerProposal != SamplerProposal::eFixe) {
            generateCorrelCurves(mChains, &(model->mLambdaSpline));
            mGraph->setXScaleDivision(10, 10);
        }
        else
            mGraph->resetNothingMessage();

    } else  {
        mTitle = tr("Smoothing");
        mGraph->resetNothingMessage();
    }
}

void GraphViewLambda::updateCurvesToShow(bool showAllChains, const QList<bool>& showChainList, const QList<variable_t> &showList)
{
    updateCurves(mCurrentTypeGraph, showList, showAllChains, showChainList);
    return;


    GraphViewResults::updateCurvesToShow(showAllChains, showChainList, showList);

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


        mGraph->setTipXLab(tr("Log10 Smoothing"));

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
        mGraph->setTipYLab(tr("Log10 Smoothing"));

        mGraph->setXAxisSupport(AxisTool::AxisSupport::eAllways_Positive);
        mGraph->setYAxisSupport(AxisTool::AxisSupport::eMin_Max);
        mGraph->setYAxisMode(GraphView::AxisMode::eMinMaxHidden);
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

        mGraph->setYAxisMode(GraphView::AxisMode::eMinMax );
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

          mGraph->setYAxisMode(GraphView::AxisMode::eMinMax);
          mGraph->showInfos(false);
          mGraph->clearInfos();
          mGraph->autoAdjustYScale(false); // do  repaintGraph()
          mGraph->setRangeY(-1, 1);
      }

      repaint();
}


void GraphViewLambda::generatePosterior()
{
    auto model = getModel_ptr();

    QColor color = Qt::blue;
    QPen defaultPen;
    defaultPen.setWidthF(1);
    defaultPen.setStyle(Qt::SolidLine);

    mGraph->mLegendX = "Log10";
    mGraph->setFormatFunctX(nullptr);
    mGraph->setFormatFunctY(nullptr);
    mGraph->setBackgroundColor(QColor(230, 230, 230));
    mGraph->setOverArrow(GraphView::OverflowDataArrowMode::eBothOverflow);

    mGraph->autoAdjustYScale(true);

    mGraph->setXAxisMode(GraphView::AxisMode::eAllTicks);
    mGraph->setYAxisMode(GraphView::AxisMode::eHidden);

    mTitle = tr("Smoothing");

    // ------------------------------------
    //  Post distrib All Chains
    // ------------------------------------
    if (mShowAllChains) {
        const GraphCurve &curvePostDistrib = densityCurve(model->mLambdaSpline.mFormatedKDE,
                                                          "Post Distrib All Chains",
                                                          color);

        mGraph->add_curve(curvePostDistrib);

        // ------------------------------------
        //  HPD All Chains
        // ------------------------------------
        const GraphCurve &curveHPD = HPDCurve(model->mLambdaSpline.mFormatedHPD, "HPD All Chains", color);
        mGraph->add_curve(curveHPD);

        if (mShowList.contains(eCredibility)) {
            const GraphCurve &curveCred = topLineSection(model->mLambdaSpline.mFormatedCredibility,
                                                         "Credibility All Chains",
                                                         color);
            mGraph->add_curve(curveCred);
        }
    }
    // ------------------------------------
    //  Post Distrib Chain i
    // ------------------------------------

    for (size_t i = 0; i < mChains.size(); ++i)  {
        if (mShowChainList[i]) {
            const GraphCurve &curvePostDistribChain = densityCurve(model->mLambdaSpline.KDEForChain(i),
                                                                   "Post Distrib Chain " + QString::number(i),
                                                                   Painting::chainColors.at(i),
                                                                   Qt::SolidLine,
                                                                   Qt::NoBrush);
            mGraph->add_curve(curvePostDistribChain);
        }
    }

    mGraph->autoAdjustYScale(true);

}


void GraphViewLambda::generateHistory()
{
    graph_trace();
    auto model = getModel_ptr();

    mGraph->setTipYLab("Smoothing");
    mGraph->setFormatFunctX(nullptr);
    mTitle = tr("Smoothing Trace");
    if (model->mLambdaSpline.mSamplerProposal != SamplerProposal::eFixe)
        generateTraceCurves(mChains, &(model->mLambdaSpline));
    else
        mGraph->resetNothingMessage();
}

void GraphViewLambda::generateAcceptation()
{
    graph_acceptation();
    auto model = getModel_ptr();

    mGraph->setTipYLab("Rate");
    mGraph->setFormatFunctX(nullptr);
    mTitle = tr("Smoothing");
    if (model->mLambdaSpline.mSamplerProposal != SamplerProposal::eFixe)
        generateAcceptCurves(mChains, &(model->mLambdaSpline));
    else
        mGraph->resetNothingMessage();


}

void GraphViewLambda::generateCorrelation()
{
    graph_correlation();
    auto model = getModel_ptr();

    mGraph->mLegendX = "";
    mGraph->setFormatFunctX(nullptr);
    mTitle = tr("Smoothing");
    if (model->mLambdaSpline.mSamplerProposal != SamplerProposal::eFixe) {
        generateCorrelCurves(mChains, &(model->mLambdaSpline));

    }
    else
        mGraph->resetNothingMessage();

}

void GraphViewLambda::updateStatHTML()
{
    auto model = getModel_ptr();
    const QString resultsHTML = ModelUtilities::lambdaResultsHTML(model);
    setNumericalResults(resultsHTML);

}