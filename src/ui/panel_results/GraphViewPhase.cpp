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

#include "GraphViewPhase.h"
#include "GraphView.h"
#include "Phase.h"
#include "Painting.h"
#include "StdUtilities.h"
#include "QtUtilities.h"
#include "DateUtils.h"
#include "ModelUtilities.h"
#include "MainWindow.h"
#include "Button.h"
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
    setItemTitle(tr("Phase : %1").arg(mPhase->mName));

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

void GraphViewPhase::generateCurves(const graph_t typeGraph, const QVector<variable_t>& variableList)
{
    //qDebug()<<"GraphViewPhase::generateCurves()";
    Q_ASSERT(mPhase);
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

    QString resultsText = ModelUtilities::phaseResultsText(mPhase);
    QString resultsHTML = ModelUtilities::phaseResultsHTML(mPhase);
    setNumericalResults(resultsHTML, resultsText);

    mGraph->setOverArrow(GraphView::eNone);

    /* -------------first tab : posterior distrib-----------------------------------
     *  Possible curves :
     *  - Post Distrib Alpha All Chains
     *  - Post Distrib Beta All Chains
     *  - HPD Alpha All Chains
     *  - HPD Beta All Chains
     *  - Duration
     *  - HPD Duration
     *  - Post Distrib Alpha i
     *  - Post Distrib Beta i
     *  - Time Range
     *  - Post Distrib Duration
     *  - Post Distrib Tempo All Chains

     * ------------------------------------------------  */
    if (typeGraph == ePostDistrib && mCurrentVariableList.contains(eBeginEnd)) {
        mGraph->mLegendX = DateUtils::getAppSettingsFormatStr();
        mGraph->mLegendY = "";
        mGraph->setFormatFunctX(nullptr);//DateUtils::convertToAppSettingsFormat);
        mGraph->setFormatFunctY(nullptr);
         mGraph->setYAxisMode(GraphView::eMinMaxHidden);

        mTitle = tr("Phase : %1").arg(mPhase->mName);
        QMap<double,double> &alpha = mPhase->mAlpha.fullHisto();
        QMap<double,double> &beta = mPhase->mBeta.fullHisto();
        /*
         * Detection of one Bound used as boundary != is xor
         * If there is two Bound, the both are egal to 1, thus nothing to do
         */
        if ((alpha.size()==1) != (beta.size()==1)) {
            if (alpha.size() == 1)
                alpha[alpha.firstKey()] = map_max_value(beta) * 2.;
            else
                beta[beta.firstKey()] = map_max_value(alpha) * 2.;

        }

        GraphCurve curveAlpha = generateDensityCurve(alpha,
                                                     "Post Distrib Alpha All Chains",
                                                     color, Qt::DotLine);
        QColor colorBeta = mPhase->mColor.darker(170);


        GraphCurve curveBeta = generateDensityCurve(beta,
                                                     "Post Distrib Beta All Chains",
                                                     colorBeta, Qt::DashLine);
        color.setAlpha(255); // set mBrush to fill
        GraphCurve curveAlphaHPD = generateHPDCurve(mPhase->mAlpha.mHPD,
                                                     "HPD Alpha All Chains",
                                                     color);

        GraphCurve curveBetaHPD = generateHPDCurve(mPhase->mBeta.mHPD,
                                                   "HPD Beta All Chains",
                                                   colorBeta);

        mGraph->addCurve(curveAlpha);
        mGraph->addCurve(curveBeta);


        mGraph->addCurve(curveAlphaHPD);
        mGraph->addCurve(curveBetaHPD);

        mGraph->setOverArrow(GraphView::eBothOverflow);

        GraphCurve curveTimeRange = generateSectionCurve(mPhase->getFormatedTimeRange(),
                                                                   "Time Range",
                                                                   color);
        mGraph->addCurve(curveTimeRange);

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

        if (!mPhase->mAlpha.mChainsHistos.isEmpty())
            for (auto i=0; i<mChains.size(); ++i) {
                GraphCurve curveAlpha = generateDensityCurve(mPhase->mAlpha.histoForChain(i),
                                                             "Post Distrib Alpha " + QString::number(i),
                                                             Painting::chainColors.at(i), Qt::DotLine);

                GraphCurve curveBeta = generateDensityCurve(mPhase->mBeta.histoForChain(i),
                                                            "Post Distrib Beta " + QString::number(i),
                                                            Painting::chainColors.at(i).darker(170), Qt::DashLine);
                mGraph->addCurve(curveAlpha);
                mGraph->addCurve(curveBeta);
            }
       // mGraph->setYAxisMode(GraphView::eMinMax);

    } else if (typeGraph == ePostDistrib && mCurrentVariableList.contains(eDuration)) {
        mGraph->mLegendX = tr("Years");
        mGraph->mLegendY = "";
        mGraph->setFormatFunctX(nullptr);
        mGraph->setFormatFunctY(nullptr);
        mGraph->setYAxisMode(GraphView::eMinMaxHidden);
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
            mGraph->setFormatFunctX(nullptr);
            mGraph->setFormatFunctY(nullptr);

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
    } else if (typeGraph == ePostDistrib && mCurrentVariableList.contains(eSigma)) {

        mGraph->mLegendX = "";
        mGraph->setFormatFunctX(nullptr);//DateUtils::convertToAppSettingsFormat);
        mGraph->setFormatFunctY(nullptr);
        mGraph->setYAxisMode(GraphView::eMinMaxHidden);
        mTitle = tr("Phase's Events' Std Compil. : %1").arg(mPhase->mName);


        for ( auto && ev : mPhase->mEvents) {
        /* ------------------------------------------------
         *  Events don't have std dev BUT we can visualize
         *  an overlay of all dates std dev instead.
         *  Possible curves, FOR ALL DATES :
         *  - Sigma Date i All Chains
         *  - Sigma Date i Chain j
         * ------------------------------------------------
         */
            int i(0);
            for (auto&& date : ev->mDates) {
                GraphCurve curve = generateDensityCurve(date.mSigmaTi.fullHisto(),
                                                        "Sigma Date " + QString::number(i) + " All Chains",
                                                        color);

                mGraph->addCurve(curve);
                if (!date.mSigmaTi.mChainsHistos.isEmpty())
                    for (int j=0; j<mChains.size(); ++j) {
                        GraphCurve curveChain = generateDensityCurve(date.mSigmaTi.histoForChain(j),
                                                                     "Sigma Date " + QString::number(i) + " Chain " + QString::number(j),
                                                                     Painting::chainColors.at(j));
                        mGraph->addCurve(curveChain);
                    }
                ++i;
            }

        }

    }

    else if (typeGraph == ePostDistrib && mCurrentVariableList.contains(eTempo)) {
        mGraph->mLegendX = DateUtils::getAppSettingsFormatStr();
        mGraph->mLegendY = "";
        mGraph->setFormatFunctX(nullptr);//DateUtils::convertToAppSettingsFormat);
        mGraph->setFormatFunctY(nullptr);
        mGraph->setYAxisMode(GraphView::eMinMax);

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
/*
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
*/
        mGraph->addCurve(curveTempoInf);
        mGraph->addCurve(curveTempo);
        mGraph->addCurve(curveTempoSup);

  //      mGraph->addCurve(curveCredInf);
   //     mGraph->addCurve(curveCredSup);

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

        const type_data yMax = map_max_value(curveTempoSup.mData);

        mGraph->setRangeY(0., yMax);

    } else if (typeGraph == ePostDistrib && mCurrentVariableList.contains(eActivity)) {

        mGraph->mLegendX = DateUtils::getAppSettingsFormatStr();
        mGraph->setFormatFunctX(nullptr);
        mGraph->setFormatFunctY(nullptr);
        mGraph->setYAxisMode(GraphView::eMinMax);

        mTitle = tr("Phase Activity : %1").arg(mPhase->mName);
        GraphCurve curveActivity = generateDensityCurve( mPhase->mActivity,
                                                         "Post Distrib Activity All Chains",
                                                         color, Qt::SolidLine);

        GraphCurve curveActivityInf = generateDensityCurve( mPhase->mActivityInf,
                                                            "Post Distrib Activity Inf All Chains",
                                                            color, Qt::CustomDashLine);

        GraphCurve curveActivitySup = generateDensityCurve( mPhase->mActivitySup,
                                                            "Post Distrib Activity Sup All Chains",
                                                            color, Qt::CustomDashLine);

         // Display envelope Uniform
        GraphCurve curveMeanUnif = generateHorizontalLine( mPhase->mActivityMeanUnif,
                                                           "Post Distrib Activity Unif Mean",
                                                           QColor(Qt::darkGray).darker(), Qt::CustomDashLine);

        GraphCurve curveUnifInf = generateHorizontalLine( mPhase->mActivityMeanUnif - mPhase->mActivityStdUnif,
                                                          "Post Distrib Activity Unif Inf",
                                                          Qt::darkGray, Qt::CustomDashLine);
        GraphCurve curveUnifSup = generateHorizontalLine( mPhase->mActivityMeanUnif + mPhase->mActivityStdUnif,
                                                          "Post Distrib Activity Unif Sup",
                                                          Qt::darkGray, Qt::CustomDashLine);
        // Display p_value
        GraphCurve curvePValue = generateDensityCurve( mPhase->mActivityPValue,
                                                         "Post Distrib Activity p_value All Chains",
                                                         Qt::red, Qt::SolidLine);

        mGraph->setOverArrow(GraphView::eBothOverflow);


        mGraph->addCurve(curveActivity);
        mGraph->addCurve(curveActivityInf);
        mGraph->addCurve(curveActivitySup);
        mGraph->addCurve(curveMeanUnif);
        mGraph->addCurve(curveUnifInf);
        mGraph->addCurve(curveUnifSup);
        mGraph->addCurve(curvePValue);

        const type_data yMax = map_max_value(curveActivitySup.mData);

        mGraph->setRangeY(0., yMax);

    }

    /* -----------------second tab : history plot-------------------------------
     *  - Trace Alpha i
     *  - Q1 Alpha i
     *  - Q2 Alpha i
     *  - Q3 Alpha i
     *  - Trace Beta i
     *  - Q1 Beta i
     *  - Q2 Beta i
     *  - Q3 Beta i
     * ------------------------------------------------ */
    else if (typeGraph == eTrace && mCurrentVariableList.contains(eBeginEnd)) {
        mGraph->mLegendX = tr("Iterations");
        mGraph->setFormatFunctX(nullptr);
        mGraph->setFormatFunctY(nullptr);//DateUtils::convertToAppSettingsFormat);
        mGraph->setYAxisMode(GraphView::eMinMax);
        mTitle = tr("Phase : %1").arg(mPhase->mName);

        generateTraceCurves(mChains, &(mPhase->mAlpha), "Alpha");
        generateTraceCurves(mChains, &(mPhase->mBeta), "Beta");
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
     *  - Post Distrib Alpha All Chains
     *  - Post Distrib Beta All Chains
     *  - HPD Alpha All Chains
     *  - HPD Beta All Chains
     *  - Duration
     *  - HPD Duration
     *  - Post Distrib Alpha i
     *  - Post Distrib Beta i
     *
     *  - Post Distrib Duration All Chains
     *  - HPD Duration All Chains
     *  - Credibility All Chains
     *  - Post Distrib Tempo All Chains
     *  - Post Distrib Tempo Inf All Chain
     *  - Post Distrib Tempo Sup All Chain
     *  - Post Distrib Tempo Cred Inf All Chains // obsolete
     *  - Post Distrib Tempo Cred Sup All Chains // obsolete
     * ------------------------------------------------
     */
    if (mCurrentTypeGraph == ePostDistrib && mShowVariableList.contains(eBeginEnd)) {
        const bool showCredibility = true;
        mGraph->setTipYLab("");

        mGraph->setCurveVisible("Post Distrib Alpha All Chains", mShowAllChains);
        mGraph->setCurveVisible("Post Distrib Beta All Chains", mShowAllChains);
        mGraph->setCurveVisible("HPD Alpha All Chains", mShowAllChains);
        mGraph->setCurveVisible("HPD Beta All Chains", mShowAllChains);

        mGraph->setCurveVisible("Time Range", mShowAllChains && showCredibility);

        for (auto i=0; i<mShowChainList.size(); ++i) {
            mGraph->setCurveVisible("Post Distrib Alpha " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Post Distrib Beta " + QString::number(i), mShowChainList.at(i));
        }

        mGraph->setTipXLab("t");
        mGraph->setTipYLab("");
        //mGraph->setYAxisMode(GraphView::eHidden);
        mGraph->showInfos(false);
        mGraph->clearInfos();
        mGraph->autoAdjustYScale(true); // do repaintGraph()

    }

    else if (mCurrentTypeGraph == ePostDistrib && mShowVariableList.contains(eSigma)) {
            for (auto && ev : mPhase->mEvents) {
                auto n = ev->mDates.size();
                for (auto i = 0 ; i<n; ++i) {
                    mGraph->setCurveVisible("Sigma Date " + QString::number(i) + " All Chains", mShowAllChains);

                   for (int j=0; j<mShowChainList.size(); ++j)
                        mGraph->setCurveVisible("Sigma Date " + QString::number(i) + " Chain " + QString::number(j), mShowChainList.at(j));

                }
            }
            mGraph->setTipXLab("Sigma");
           // mGraph->setYAxisMode(GraphView::eHidden);
            mGraph->autoAdjustYScale(true);

    }
    else if (mCurrentTypeGraph == ePostDistrib && mShowVariableList.contains(eDuration)) {
        const GraphCurve* duration = mGraph->getCurve("Post Distrib Duration All Chains");

        if ( duration && !duration->mData.isEmpty()) {
            const bool showCredibility = true;
            mGraph->setCurveVisible("Post Distrib Duration All Chains", mShowAllChains);
            mGraph->setCurveVisible("HPD Duration All Chains", mShowAllChains);
            mGraph->setCurveVisible("Credibility All Chains", showCredibility && mShowAllChains);

            for (unsigned i=0; i<mShowChainList.size(); ++i)
                mGraph->setCurveVisible("Post Distrib Duration " + QString::number(i), mShowChainList.at(i));

            mGraph->setTipXLab("t");
            mGraph->setTipYLab("");
            //mGraph->setYAxisMode(GraphView::eHidden);
            mGraph->autoAdjustYScale(true);
        }

    }
    else if (mCurrentTypeGraph == ePostDistrib && mShowVariableList.contains(eTempo)) {
    // With variable eTemp there is no choice of "chain", it must be "all chains"
         const GraphCurve* tempo = mGraph->getCurve("Post Distrib Tempo All Chains");

         if ( tempo && !tempo->mData.isEmpty()) {
             const bool showError = mShowVariableList.contains(eTempoError);
             //const bool showCredibility = mShowVariableList.contains(eTempCredibility);
             mGraph->setCurveVisible("Post Distrib Tempo All Chains", true);
             mGraph->setCurveVisible("Post Distrib Tempo Inf All Chains", showError);
             mGraph->setCurveVisible("Post Distrib Tempo Sup All Chains", showError);

             //mGraph->setCurveVisible("Post Distrib Tempo Cred Inf All Chains", showCredibility);
             //mGraph->setCurveVisible("Post Distrib Tempo Cred Sup All Chains", showCredibility);

             mGraph->setTipXLab("t");
             mGraph->setTipYLab("n");
            // mGraph->setYAxisMode(GraphView::eMinMax);
             mGraph->autoAdjustYScale(true);// do repaintGraph()
         }

    }
    else if (mCurrentTypeGraph == ePostDistrib && mShowVariableList.contains(eActivity)) {
        // With variable eActivity there is no choice of "chain", it must be "all chains"
          const GraphCurve* Activity = mGraph->getCurve("Post Distrib Activity All Chains");

          if ( Activity && !Activity->mData.isEmpty()) {
              const bool showError = mShowVariableList.contains(eTempoError);//true;
              mGraph->setCurveVisible("Post Distrib Activity All Chains", true);
              mGraph->setCurveVisible("Post Distrib Activity Inf All Chains", showError);
              mGraph->setCurveVisible("Post Distrib Activity Sup All Chains", showError);

              // enveloppe Uniforne
              mGraph->setCurveVisible("Post Distrib Activity Unif Mean", true);
              mGraph->setCurveVisible("Post Distrib Activity Unif Inf", showError);
              mGraph->setCurveVisible("Post Distrib Activity Unif Sup", showError);

              // p_value curve
              mGraph->setCurveVisible("Post Distrib Activity p_value All Chains", showError);

              mGraph->setTipXLab("t");
              mGraph->setTipYLab("n");
              //mGraph->setYAxisMode(GraphView::eHidden);
              mGraph->autoAdjustYScale(true); // do repaintGraph()
          }

     }

    /* ---------------- second tab : history plot--------------------------------
     *  - Alpha Trace i
     *  - Alpha Q1 i
     *  - Alpha Q2 i
     *  - Alpha Q3 i
     *  - Beta Trace i
     *  - Beta Q1 i
     *  - Beta Q2 i
     *  - Beta Q3 i
     *
     *  - Duration Trace i
     *  - Duration Q1 i
     *  - Duration Q2 i
     *  - Duration Q3 i
     * ------------------------------------------------ */
    else if (mCurrentTypeGraph == eTrace && mShowVariableList.contains(eBeginEnd)) {

        for (int i=0; i<mShowChainList.size(); ++i) {
            mGraph->setCurveVisible("Alpha Trace " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Alpha Q1 " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Alpha Q2 " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Alpha Q3 " + QString::number(i), mShowChainList.at(i));

            mGraph->setCurveVisible("Beta Trace " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Beta Q1 " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Beta Q2 " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Beta Q3 " + QString::number(i), mShowChainList.at(i));
        }

        mGraph->setTipXLab(tr("Iteration"));
        mGraph->setTipYLab("t");
        mGraph->setYAxisMode(GraphView::eMinMaxHidden);
        mGraph->showInfos(true);
        mGraph->autoAdjustYScale(true);
    } else if (mCurrentTypeGraph == eTrace && mShowVariableList.contains(eDuration)) {

        for (int i=0; i<mShowChainList.size(); ++i) {
            mGraph->setCurveVisible("Duration Trace " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Duration Q1 " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Duration Q2 " + QString::number(i), mShowChainList.at(i));
            mGraph->setCurveVisible("Duration Q3 " + QString::number(i), mShowChainList.at(i));
        }

        mGraph->setTipXLab(tr("Iteration"));
        mGraph->setTipYLab("t");
        //mGraph->setYAxisMode(GraphView::eMinMax);
        mGraph->autoAdjustYScale(true); // do repaintGraph()
    }

    repaint();
}
