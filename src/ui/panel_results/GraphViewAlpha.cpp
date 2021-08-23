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

#include "GraphViewAlpha.h"
#include "GraphView.h"
#include "ModelCurve.h"
#include "Painting.h"
#include "StdUtilities.h"
#include "QtUtilities.h"
#include "DateUtils.h"
#include "ModelUtilities.h"
#include "MainWindow.h"
#include <QtWidgets>

// Constructor / Destructor

GraphViewAlpha::GraphViewAlpha(QWidget *parent):GraphViewResults(parent),
mModel(nullptr)
{
    setMainColor(Painting::borderDark);
    mGraph->setBackgroundColor(QColor(210, 210, 210));

}

GraphViewAlpha::~GraphViewAlpha()
{
    mModel = nullptr;
}


void GraphViewAlpha::setModel(ModelCurve* model)
{
    Q_ASSERT(model);
    mModel = model;
}


void GraphViewAlpha::paintEvent(QPaintEvent* e)
{
    GraphViewResults::paintEvent(e);
}

void GraphViewAlpha::resizeEvent(QResizeEvent* )
{
    updateLayout();
}

void GraphViewAlpha::generateCurves(const graph_t typeGraph, const QVector<variable_t> &variableList)
{
    Q_ASSERT(mModel);

    GraphViewResults::generateCurves(typeGraph, variableList);
    
    mGraph->removeAllCurves();
    mGraph->removeAllZones();
    mGraph->clearInfos();
    mGraph->resetNothingMessage();
    mGraph->setOverArrow(GraphView::eNone);
    mGraph->reserveCurves(6);

    QPen defaultPen;
    defaultPen.setWidthF(1);
    defaultPen.setStyle(Qt::SolidLine);

    QColor color = Qt::blue;

    // ------------------------------------------------
    //  First tab : Posterior distrib
    // ------------------------------------------------
    if (typeGraph == ePostDistrib) {
        mGraph->mLegendX = "Log10";
        mGraph->setFormatFunctX(nullptr);
        mGraph->setFormatFunctY(nullptr);
        mGraph->setBackgroundColor(QColor(230, 230, 230));
        mGraph->setOverArrow(GraphView::eBothOverflow);
        
        mTitle = tr("Lambda Spline");

        // ------------------------------------
        //  Post distrib All Chains
        // ------------------------------------

        GraphCurve curvePostDistrib;
        curvePostDistrib.mName = "Post Distrib All Chains";
        curvePostDistrib.mData = mModel->mLambdaSpline.fullHisto();
        curvePostDistrib.mPen = QPen(color, 1, Qt::SolidLine);
        curvePostDistrib.mBrush = Qt::NoBrush;
        curvePostDistrib.mIsHisto = false;
        //curvePostDistrib.mIsRectFromZero = true; // for Unif-typo. calibs., invisible for others!
        
        mGraph->addCurve(curvePostDistrib);

        // ------------------------------------
        //  HPD All Chains
        // ------------------------------------
        GraphCurve curveHPD = generateHPDCurve(mModel->mLambdaSpline.mHPD, "HPD All Chains", color);
        mGraph->addCurve(curveHPD);

        // ------------------------------------
        //  Post Distrib Chain i
        // ------------------------------------
        if (!mModel->mLambdaSpline.mChainsHistos.isEmpty()) {
            for (int i=0; i<mChains.size(); ++i)  {
                GraphCurve curvePostDistribChain = generateDensityCurve(mModel->mLambdaSpline.histoForChain(i),
                                                                        "Post Distrib Chain " + QString::number(i),
                                                                        Painting::chainColors.at(i),
                                                                        Qt::SolidLine,
                                                                        Qt::NoBrush);
                mGraph->addCurve(curvePostDistribChain);
            }
        }
        
        // ------------------------------------
        //  Theta Credibility
        // ------------------------------------
        GraphCurve curveCred = generateSectionCurve(mModel->mLambdaSpline.mCredibility,
                                                    "Credibility All Chains",
                                                    color);
        mGraph->addCurve(curveCred);
        mGraph->autoAdjustYScale(true);
    }
    // -------------------------------------------------
    //  History plots
    // -------------------------------------------------
    else if (typeGraph == eTrace) {
        mGraph->mLegendX = "Iterations";
        mGraph->setTipYLab("Lambda");
        mGraph->setFormatFunctX(nullptr);
        mTitle = tr("Lambda Spline Trace");

        generateTraceCurves(mChains, &(mModel->mLambdaSpline));
    }
    // -------------------------------------------------
    //  Acceptance rate
    // -------------------------------------------------
    else if (typeGraph == eAccept) {
        mGraph->mLegendX = "Iterations";
        mGraph->setTipYLab("Rate");
        mGraph->setFormatFunctX(nullptr);
        mTitle = tr("Lambda Spline Acceptation");

        generateAcceptCurves(mChains, &(mModel->mLambdaSpline));
        mGraph->repaint();
    }

    // -------------------------------------------------
    //  Autocorrelation
    // -------------------------------------------------
    else if (typeGraph == eCorrel) {
        mGraph->mLegendX = "";
        mGraph->setFormatFunctX(nullptr);
        mTitle = tr("Lambda Spline Autocorrelation");

        generateCorrelCurves(mChains, &(mModel->mLambdaSpline));
        mGraph->setXScaleDivision(10, 10);

    } else  {
        mTitle = tr("Lambda Spline");
        mGraph->resetNothingMessage();
    }
}

void GraphViewAlpha::updateCurvesToShow(bool showAllChains, const QList<bool>& showChainList, const QVector<variable_t>& variableList)
{
    Q_ASSERT(mModel);

    GraphViewResults::updateCurvesToShow(showAllChains, showChainList, variableList);

    if (mCurrentTypeGraph == ePostDistrib)  {
        mGraph->setTipYLab("");
        
        mGraph->setCurveVisible("Post Distrib All Chains", mShowAllChains);
        mGraph->setCurveVisible("HPD All Chains", mShowAllChains);
        mGraph->setCurveVisible("Credibility All Chains", variableList.contains(eTempCredibility) && mShowAllChains);

        for (int i=0; i<mShowChainList.size(); ++i) {
            mGraph->setCurveVisible("Post Distrib Chain " + QString::number(i), mShowChainList.at(i));
        }
        
        mGraph->setTipXLab("t");
        mGraph->setYAxisMode(GraphView::eHidden);
        mGraph->showInfos(false);
        mGraph->clearInfos();
    }

    else if (mCurrentTypeGraph == eTrace)  {
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

      update();
}
