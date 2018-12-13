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

#include "GraphViewTempo.h"
#include "GraphView.h"
#include "Phase.h"
#include "Painting.h"
#include "StdUtilities.h"
#include "QtUtilities.h"
#include "DateUtils.h"
#include "ModelUtilities.h"
#include "MainWindow.h"
#include <QtWidgets>

// Constructor / Destructor

GraphViewTempo::GraphViewTempo(QWidget *parent):GraphViewResults(parent),
mPhase(nullptr)
{
    setMainColor(Painting::borderDark);
    mGraph->setBackgroundColor(QColor(210, 210, 210));

}

GraphViewTempo::~GraphViewTempo()
{
    mPhase = nullptr;
}


void GraphViewTempo::setPhase(Phase* phase)
{
    Q_ASSERT(phase);

    mPhase = phase;
    setItemTitle(tr("Phase : %1").arg(mPhase->mName));

    setItemColor(mPhase->mColor);

}


void GraphViewTempo::paintEvent(QPaintEvent* e)
{
    GraphViewResults::paintEvent(e);
}

void GraphViewTempo::resizeEvent(QResizeEvent* )
{
    updateLayout();
}

void GraphViewTempo::generateCurves(TypeGraph typeGraph, Variable variable)
{
    qDebug()<<"GraphViewTempo::generateCurves()";
    Q_ASSERT(mPhase);
    GraphViewResults::generateCurves(typeGraph, variable);

    mGraph->removeAllCurves();
    mGraph->reserveCurves(9);

    mGraph->removeAllZones();
    mGraph->clearInfos();
    mGraph->resetNothingMessage();

    QPen defaultPen;
    defaultPen.setWidthF(1.);
    defaultPen.setStyle(Qt::SolidLine);

    QColor color = mPhase->mColor;

    QString resultsText = ModelUtilities::tempoResultsText(mPhase);
    QString resultsHTML = ModelUtilities::tempoResultsHTML(mPhase);
    setNumericalResults(resultsHTML, resultsText);

    mGraph->setOverArrow(GraphView::eNone); // ??
    /* -------------first tab : posterior distrib-----------------------------------
     *  Possible curves :
     *  - Post Distrib Duration
     *  - Post Distrib Tempo All Chains
     *  - Time Range
     * ------------------------------------------------  */

    if ((typeGraph == ePostDistrib) && (variable == eDuration)) {
        mGraph->mLegendX = tr("Years");
        mGraph->mLegendY = "";
        mGraph->setFormatFunctX(nullptr);//DateUtils::convertToAppSettingsFormat);
        mGraph->setFormatFunctY(nullptr);

        mTitle = tr("Phase Duration : %1").arg(mPhase->mName);
        GraphCurve curveDuration;

        if (mPhase->mDuration.fullHisto().size()>1) {
            curveDuration = generateDensityCurve(mPhase->mDuration.fullHisto(),
                                                            "Post Distrib Duration All Chains",
                                                            color);

            mGraph->setRangeX(0., ceil(curveDuration.mData.lastKey()));
            color.setAlpha(255);
            GraphCurve curveDurationHPD = generateHPDCurve(mPhase->mDuration.mHPD,
                                                           "HPD Duration All Chains",
                                                           color);
            mGraph->setCanControlOpacity(true);
            mGraph->addCurve(curveDurationHPD);
            mGraph->setFormatFunctX(nullptr);//DateUtils::convertToAppSettingsFormat);
            mGraph->setFormatFunctY(nullptr);
            mGraph->setOverArrow(GraphView::eBothOverflow);
            mGraph->addCurve(curveDuration);


            /* ------------------------------------
             *  Theta Credibility
             * ------------------------------------
             */
            GraphCurve curveCred = generateSectionCurve(mPhase->mDuration.mCredibility,
                                                            "Credibility All Chains",
                                                            color);
            mGraph->addCurve(curveCred);

        } else
            mGraph->resetNothingMessage();


        if (!mPhase->mDuration.mChainsHistos.isEmpty())
            for (int i=0; i<mChains.size(); ++i) {
                GraphCurve curveDuration = generateDensityCurve(mPhase->mDuration.histoForChain(i),
                                                             "Post Distrib Duration " + QString::number(i),
                                                             Painting::chainColors.at(i), Qt::DotLine);

                mGraph->addCurve(curveDuration);
            }
     }

    else if ((typeGraph == ePostDistrib) && (variable == eTempo)) {
        mGraph->mLegendX = DateUtils::getAppSettingsFormatStr();
        mGraph->mLegendY = "";
        mGraph->setFormatFunctX(nullptr);//DateUtils::convertToAppSettingsFormat);
        mGraph->setFormatFunctY(nullptr);
        mTitle = tr("Phase Tempo : %1").arg(mPhase->mName);

        GraphCurve curveTempo = generateDensityCurve(mPhase->mTempo,
                                                     "Post Distrib Tempo All Chains",
                                                     color.darker(), Qt::SolidLine);
        curveTempo.mIsRectFromZero = false;
        curveTempo.mIsHisto = false;

        GraphCurve curveTempoInf = generateDensityCurve(mPhase->mTempoInf,
                                                     "Post Distrib Tempo Inf All Chains",
                                                     color, Qt::DashLine);
        curveTempoInf.mIsRectFromZero = false;
        curveTempoInf.mIsHisto = false;

        GraphCurve curveTempoSup = generateDensityCurve(mPhase->mTempoSup,
                                                     "Post Distrib Tempo Sup All Chains",
                                                     color, Qt::DashLine);
        curveTempoSup.mIsRectFromZero = false;
        curveTempoSup.mIsHisto = false;

        GraphCurve curveCredInf = generateDensityCurve(mPhase->mTempoCredibilityInf,
                                                     "Post Distrib Tempo Cred Inf All Chains",
                                                     color, Qt::SolidLine);
        curveCredInf.mIsRectFromZero = false;
        curveCredInf.mIsHisto = false;

        GraphCurve curveCredSup = generateDensityCurve(mPhase->mTempoCredibilitySup,
                                                     "Post Distrib Tempo Cred Sup All Chains",
                                                     color, Qt::SolidLine);
        curveCredSup.mIsRectFromZero = false;
        curveCredSup.mIsHisto = false;

        mGraph->addCurve(curveTempoInf);
        mGraph->addCurve(curveTempo);
        mGraph->addCurve(curveTempoSup);

        mGraph->addCurve(curveCredInf);
        mGraph->addCurve(curveCredSup);

        mGraph->setOverArrow(GraphView::eBothOverflow);

        /* ------------------------------------------------------------
        *   Add zones outside study period
        * ------------------------------------------------------------*/

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

        const type_data yMax = map_max_value(curveCredSup.mData);

        mGraph->setRangeY(0., yMax);

    }

    else if (typeGraph == ePostDistrib && variable == eActivity) {

        mGraph->mLegendX = DateUtils::getAppSettingsFormatStr();
        mGraph->setFormatFunctX(nullptr);//DateUtils::convertToAppSettingsFormat);
        mGraph->setFormatFunctY(nullptr);

        mTitle = tr("Phase Activity : %1").arg(mPhase->mName);
        GraphCurve curveActivity = generateDensityCurve(mPhase->mActivity,
                                                     "Post Distrib Activity All Chains",
                                                     color, Qt::SolidLine);

        mGraph->setOverArrow(GraphView::eBothOverflow);
        mGraph->addCurve(curveActivity);
        const type_data yMax = map_max_value(curveActivity.mData);

        mGraph->setRangeY(0., yMax);

    }

    /* -----------------Second tab : History plot-------------------------------
     *  - Duration
     * ------------------------------------------------ */

    else if (typeGraph == eTrace && variable == eDuration) {
        mGraph->mLegendX = tr("Iterations");
        mGraph->setFormatFunctX(nullptr);
        mGraph->setFormatFunctY(nullptr);//DateUtils::convertToAppSettingsFormat);
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

void GraphViewTempo::updateCurvesToShow(bool showAllChains, const QList<bool>& showChainList, bool showCredibility, bool showError, bool showWiggle)
{
    Q_ASSERT(mPhase);
    GraphViewResults::updateCurvesToShow(showAllChains, showChainList, showCredibility, showError, showWiggle);

    /* --------------------first tab : posterior distrib----------------------------
     *
     *  Possible curves :
     *  - Post Distrib Duration All Chains
     *  - HPD Duration All Chains
     *  - Credibility All Chains
     * - Post Distrib Tempo All Chains
     *  - Post Distrib Tempo Inf All Chain
     *  - Post Distrib Tempo Sup All Chain
     *  - Post Distrib Tempo Cred Inf All Chains
     *  - Post Distrib Tempo Cred Sup All Chains
     * ------------------------------------------------
     */
     if (mCurrentTypeGraph == ePostDistrib && mCurrentVariable == eDuration) {
        const GraphCurve* duration = mGraph->getCurve("Post Distrib Duration All Chains");

        if ( duration && !duration->mData.isEmpty()) {

            mGraph->setCurveVisible("Post Distrib Duration All Chains", mShowAllChains);
            mGraph->setCurveVisible("HPD Duration All Chains", mShowAllChains);
            mGraph->setCurveVisible("Credibility All Chains", mShowCredibility && mShowAllChains);

            for (int i=0; i<mShowChainList.size(); ++i)
                mGraph->setCurveVisible("Post Distrib Duration " + QString::number(i), mShowChainList.at(i));

            mGraph->setTipXLab("t");
            mGraph->setTipYLab("");
            mGraph->setYAxisMode(GraphView::eHidden);
            mGraph->autoAdjustYScale(true);
        }

    }
    else if (mCurrentTypeGraph == ePostDistrib && mCurrentVariable == eTempo) {
    // With variable eTemp there is no choice of "chain", it must be "all chains"
         const GraphCurve* tempo = mGraph->getCurve("Post Distrib Tempo All Chains");

         if ( tempo && !tempo->mData.isEmpty()) {
             mGraph->setCurveVisible("Post Distrib Tempo All Chains", true);
             mGraph->setCurveVisible("Post Distrib Tempo Inf All Chains", showError);
             mGraph->setCurveVisible("Post Distrib Tempo Sup All Chains", showError);

             mGraph->setCurveVisible("Post Distrib Tempo Cred Inf All Chains", showCredibility);
             mGraph->setCurveVisible("Post Distrib Tempo Cred Sup All Chains", showCredibility);

             mGraph->setTipXLab("t");
             mGraph->setTipYLab("n");
             mGraph->setYAxisMode(GraphView::eMinMax);
             mGraph->autoAdjustYScale(true);// do repaintGraph()
         }

    }
     else if (mCurrentTypeGraph == ePostDistrib && mCurrentVariable == eActivity) {
        // With variable eActivity there is no choice of "chain", it must be "all chains"
          const GraphCurve* Activity = mGraph->getCurve("Post Distrib Activity All Chains");

          if ( Activity && !Activity->mData.isEmpty()) {

              mGraph->setCurveVisible("Post Distrib Activity All Chains", true);

              mGraph->setTipXLab("t");
              mGraph->setTipYLab("");
              mGraph->setYAxisMode(GraphView::eHidden);
              mGraph->autoAdjustYScale(true); // do repaintGraph()
          }

     }
    /* ---------------- Second tab : History plot--------------------------------
     *  - Duration Trace i
     *  - Duration Q1 i
     *  - Duration Q2 i
     *  - Duration Q3 i
     * ------------------------------------------------ */
    else if (mCurrentTypeGraph == eTrace && mCurrentVariable == eDuration) {

        for (int i=0; i<mShowChainList.size(); ++i) {
            mGraph->setCurveVisible("Duration Trace " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Duration Q1 " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Duration Q2 " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Duration Q3 " + QString::number(i), mShowChainList.at(i));
        }

        mGraph->setTipXLab(tr("Iteration"));
        mGraph->setTipYLab("t");
        mGraph->setYAxisMode(GraphView::eMinMax);
        mGraph->autoAdjustYScale(true); // do repaintGraph()
    }
    repaint();
}
