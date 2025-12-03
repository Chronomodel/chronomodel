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
    mGraph->setOverArrow(GraphView::eNone);
    mGraph->reserveCurves(6);

    QPen defaultPen;
    defaultPen.setWidthF(1);
    defaultPen.setStyle(Qt::SolidLine);

    QColor color = Qt::blue;

    const QString resultsHTML = ModelUtilities::S02VgResultsHTML(model);
    setNumericalResults(resultsHTML);
#ifdef KOMLAN
    // ------------------------------------------------
    //  First tab : Posterior distrib
    // ------------------------------------------------
    if (typeGraph == ePostDistrib) {
        mGraph->mLegendX = "S02Vg";
        mGraph->setFormatFunctX(nullptr);
        mGraph->setFormatFunctY(nullptr);
        mGraph->setBackgroundColor(QColor(230, 230, 230));
        mGraph->setOverArrow(GraphView::eBothOverflow);

        //mGraph->setXAxisSupport(AxisTool::AxisSupport::eAllTip);
        //mGraph->setYAxisSupport(AxisTool::AxisSupport::eAllways_Positive);

        mGraph->autoAdjustYScale(true);

        mGraph->setXAxisMode(GraphView::eAllTicks);
        mGraph->setYAxisMode(GraphView::eHidden);



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
            const GraphCurve &curvePostDistribChain = densityCurve(model->mS02Vg.histoForChain(i),
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
            if (mShowVariableList.contains(eCredibility))
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
