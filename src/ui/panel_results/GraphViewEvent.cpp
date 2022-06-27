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

#include "GraphViewEvent.h"
#include "GraphView.h"
#include "Event.h"
#include "EventBound.h"
#include "StdUtilities.h"
#include "QtUtilities.h"
#include "ModelUtilities.h"
#include "Painting.h"
#include "MainWindow.h"


#include <QtWidgets>


// Constructor / Destructor

GraphViewEvent::GraphViewEvent(QWidget *parent):GraphViewResults(parent),
mEvent(nullptr)
{
    setMainColor(QColor(100, 100, 100));
    mGraph->setBackgroundColor(QColor(230, 230, 230));
}

GraphViewEvent::~GraphViewEvent()
{
    mEvent = nullptr;
}

void GraphViewEvent::setEvent(Event* event)
{
    Q_ASSERT(event);

    mEvent = event;
    QString eventTitle = ( (mEvent->mType == Event::eDefault) ? tr("Event : %1").arg(mEvent->mName) : tr("Bound : %1").arg(mEvent->mName) ) ;
    //setItemTitle(eventTitle);
    setItemColor(mEvent->mColor);
    update();
}


void GraphViewEvent::paintEvent(QPaintEvent* e)
{
    GraphViewResults::paintEvent(e);
}

void GraphViewEvent::resizeEvent(QResizeEvent* )
{
    updateLayout();
}

void GraphViewEvent::generateCurves(const graph_t typeGraph,const QVector<variable_t> & variableList, const Model* model)
{
    Q_ASSERT(mEvent);
    (void) model;
    GraphViewResults::generateCurves(typeGraph, variableList);

    /* ------------------------------------------------
     *  Reset the graph object settings
     * ------------------------------------------------
     */
    mGraph->removeAllCurves();
    mGraph->reserveCurves(6);

    mGraph->removeAllZones();
    mGraph->clearInfos();
    mGraph->resetNothingMessage();

    mGraph->setOverArrow(GraphView::eNone);
    mGraph->setFormatFunctY(nullptr);

    QPen defaultPen;
    defaultPen.setWidthF(1);
    defaultPen.setStyle(Qt::SolidLine);

    QColor color = mEvent->mColor;

    QString resultsText = tr("Nothing to Display");
    QString resultsHTML = tr("Nothing to Display");
    if (mCurrentVariableList.contains(eVg)) {
        resultsText = ModelUtilities::VgResultsText(mEvent);
        resultsHTML = ModelUtilities::VgResultsHTML(mEvent);

    } else if (mCurrentVariableList.contains(eThetaEvent)) {
        resultsText = ModelUtilities::eventResultsText(mEvent, false);
        resultsHTML = ModelUtilities::eventResultsHTML(mEvent, false);
    }

    setNumericalResults(resultsHTML, resultsText);

    bool isFixedBound = false;
    EventKnown* bound = nullptr;
    if (mEvent->type() == Event::eBound) {
        bound = dynamic_cast<EventKnown*>(mEvent);
        isFixedBound = (bound != nullptr);
    }
    
    // --------------------------------------------------------------------
    //  The graph name depends on the currently displayed variable
    // --------------------------------------------------------------------
    if (mCurrentVariableList.contains(eThetaEvent)) {
        mTitle = ((mEvent->type()==Event::eBound) ? tr("Bound") : tr("Event")) + " : " + mEvent->mName;

    } else if (mCurrentVariableList.contains(eSigma)) {
        mTitle = ((mEvent->type() == Event::eBound) ? tr("Bound") : tr("Std Compilation")) + " : " + mEvent->mName;

    } else if (mCurrentVariableList.contains(eVg)) {
        mTitle = tr("Std G") + " : " + mEvent->mName;
    }

    // ------------------------------------------------
    //  First tab : Posterior distrib
    // ------------------------------------------------
    if (typeGraph == ePostDistrib) {
        mGraph->mLegendX = DateUtils::getAppSettingsFormatStr();
        mGraph->setBackgroundColor(QColor(230, 230, 230));

        /* ------------------------------------------------
         *  Possible curves :
         *  - Post Distrib All Chains
         *  - HPD All Chains
         *  - Credibility All Chains
         *  - Post Distrib Chain i
         * ------------------------------------------------
         */
        if (mCurrentVariableList.contains(eThetaEvent)) {
            mGraph->setOverArrow(GraphView::eBothOverflow);
            if (isFixedBound) {
                GraphCurve curveLineBound;
                curveLineBound.mName = "Post Distrib All Chains";
                curveLineBound.mPen.setColor(color);
                curveLineBound.mType = GraphCurve::eHorizontalSections;
                qreal tLower = bound->formatedFixedValue();
                qreal tUpper = tLower;
                curveLineBound.mSections.push_back(qMakePair(tLower,tUpper));
                mGraph->addCurve(curveLineBound);

                // generate theorical curves
                for (int i=0; i<mChains.size(); ++i) {
                   // GraphCurve curveLineBound;
                    curveLineBound.mName = "Post Distrib Chain " + QString::number(i);
                    curveLineBound.mPen.setColor(Painting::chainColors.at(i));

                    mGraph->addCurve(curveLineBound);
                }

            } else {


                /* ------------------------------------
                 *  Post Distrib All Chains
                 * ------------------------------------
                 */
                GraphCurve curvePostDistrib;
                curvePostDistrib.mName = "Post Distrib All Chains";
                curvePostDistrib.mPen.setColor(color);

                curvePostDistrib = densityCurve(mEvent->mTheta.fullHisto(),
                                                                       "Post Distrib All Chains",
                                                                       color);
                mGraph->addCurve(curvePostDistrib);

                // HPD All Chains
                const GraphCurve curveHPD = HPDCurve(mEvent->mTheta.mHPD,
                                                       "HPD All Chains",
                                                       color);
                mGraph->addCurve(curveHPD);

                /* ------------------------------------
                 *  Post Distrib Chain i
                 * ------------------------------------
                 */
                if (!mEvent->mTheta.mChainsHistos.isEmpty())
                    for (int i = 0; i < mChains.size(); ++i) {
                        GraphCurve curvePostDistribChain = densityCurve(mEvent->mTheta.histoForChain(i),
                                                                                "Post Distrib Chain " + QString::number(i),
                                                                                Painting::chainColors.at(i),
                                                                                Qt::SolidLine,
                                                                                Qt::NoBrush);
                        mGraph->addCurve(curvePostDistribChain);
                    }

                /* ------------------------------------
                 *  Theta Credibility
                 * ------------------------------------
                 */
                const GraphCurve curveCred = topLineSection(mEvent->mTheta.mCredibility,
                                                                "Credibility All Chains",
                                                                color);
                mGraph->addCurve(curveCred);
            }
            /* ------------------------------------------------------------
            *   Add zones outside study period
            * ------------------------------------------------------------*/

            const GraphZone zoneMin (-INFINITY, mSettings.getTminFormated());
            mGraph->addZone(zoneMin);

            const GraphZone zoneMax (mSettings.getTmaxFormated(), INFINITY);
            mGraph->addZone(zoneMax);
        }


        /* ------------------------------------------------
         *  Events don't have std dev BUT we can visualize
         *  an overlay of all dates std dev instead.
         *  Possible curves, FOR ALL DATES :
         *  - Post Distrib i All Chains
         *  - Post Distrib i Chain j
         * ------------------------------------------------
         */
        else if (mCurrentVariableList.contains(eSigma)) {
            mGraph->setOverArrow(GraphView::eNone);
            mGraph->mLegendX = "";

            mGraph->setBackgroundColor(QColor(230, 230, 230));

            int i = 0;
            for (auto&& date : mEvent->mDates) {
                GraphCurve curve = densityCurve(date.mSigmaTi.fullHisto(),
                                                        "Post Distrib Date " + QString::number(i) + " All Chains",
                                                        color);

                mGraph->addCurve(curve);
                if (!date.mSigmaTi.mChainsHistos.isEmpty())
                    for (int j=0; j<mChains.size(); ++j) {
                        GraphCurve curveChain = densityCurve(date.mSigmaTi.histoForChain(j),
                                                                     "Post Distrib Date " + QString::number(i) + " Chain " + QString::number(j),
                                                                     Painting::chainColors.at(j));
                        mGraph->addCurve(curveChain);
                    }
                ++i;
            }
        }
        
        else if (mCurrentVariableList.contains(eVg)) {
            mGraph->setOverArrow(GraphView::eNone);
            mGraph->mLegendX = "";
            mGraph->setBackgroundColor(QColor(230, 230, 230));

            GraphCurve curve = densityCurve(mEvent->mVG.fullHisto(), "Post Distrib All Chains", color);

            mGraph->addCurve(curve);
            
            if (!mEvent->mVG.mChainsHistos.isEmpty()) {
                for (int j = 0; j<mChains.size(); ++j) {
                    GraphCurve curveChain = densityCurve(mEvent->mVG.histoForChain(j), "Post Distrib Chain " + QString::number(j), Painting::chainColors.at(j));
                    mGraph->addCurve(curveChain);
                }
            }

            // HPD All Chains
            GraphCurve curveHPD = HPDCurve(mEvent->mVG.mHPD, "HPD All Chains", color);
            mGraph->addCurve(curveHPD);
        }
    }
    // ----------------------------------------------------------------------
    //  Trace : generate Trace, Q1, Q2, Q3 for all chains
    // ----------------------------------------------------------------------
    else if (typeGraph == eTrace) {
        mGraph->mLegendX = "Iterations";
        
        if (mCurrentVariableList.contains(eThetaEvent)) {
            generateTraceCurves(mChains, &(mEvent->mTheta));

        } else if (mCurrentVariableList.contains(eVg)) {
            generateTraceCurves(mChains, &(mEvent->mVG));
        }
    }
    // ----------------------------------------------------------------------
    //  Acceptance rate : generate acceptance rate and accept target curves
    // ----------------------------------------------------------------------
    else if (typeGraph == eAccept)  {
        mGraph->mLegendX = "Iterations";
        
        if (mCurrentVariableList.contains(eThetaEvent) && (mEvent->mTheta.mSamplerProposal == MHVariable::eMHAdaptGauss || mEvent->mTheta.mSamplerProposal == MHVariable::eFixe)) {
            generateAcceptCurves(mChains, &(mEvent->mTheta));

        } else if (mCurrentVariableList.contains(eVg)) {
            generateAcceptCurves(mChains, &(mEvent->mVG));

        }

    }
    // ----------------------------------------------------------------------
    //  Autocorrelation : generate correlation with lower and upper limits
    // ----------------------------------------------------------------------
    else if ((typeGraph == eCorrel) && (mCurrentVariableList.contains(eVg) || (mCurrentVariableList.contains(eThetaEvent) && !isFixedBound))) {

        mGraph->mLegendX = "";
        if (mCurrentVariableList.contains(eThetaEvent)) {
            generateCorrelCurves(mChains, &(mEvent->mTheta));

        } else if (mCurrentVariableList.contains(eVg)) {
            generateCorrelCurves(mChains, &(mEvent->mVG));
        }
        mGraph->setXScaleDivision(10, 10);

    } else {
        mTitle = ((mEvent->type()==Event::eBound) ? tr("Bound : %1").arg(mEvent->mName) : tr("Event : %1").arg(mEvent->mName));
        mGraph->resetNothingMessage();
    }

}

void GraphViewEvent::updateCurvesToShow(bool showAllChains, const QList<bool>& showChainList, const QVector<variable_t>& showVariableList)
{
    Q_ASSERT(mEvent);
    GraphViewResults::updateCurvesToShow(showAllChains, showChainList, showVariableList);

    bool isFixedBound = false;

  //  EventKnown* bound = nullptr;
    if (mEvent->type() == Event::eBound)
         isFixedBound = true;


    /* --------------------first tab : Posterior distrib----------------------------
     * ------------------------------------------------*/
    if (mCurrentTypeGraph == ePostDistrib) {
        mGraph->setTipYLab("");
        /* ------------------------------------------------
         *  Possible curves :
         *  - Post Distrib All Chains
         *  - HPD All Chains
         *  - Credibility All Chains
         *  - Post Distrib Chain i
         * ------------------------------------------------
         */
        if (mShowVariableList.contains(eThetaEvent)) {
            mGraph->setCurveVisible("Post Distrib All Chains", mShowAllChains);
            mGraph->setCurveVisible("HPD All Chains", mShowAllChains);
            mGraph->setCurveVisible("Credibility All Chains", mShowAllChains);

            for (int i = 0; i < mShowChainList.size(); ++i)
                mGraph->setCurveVisible("Post Distrib Chain " + QString::number(i), mShowChainList.at(i));

            mGraph->setTipXLab("t");
            mGraph->setYAxisMode(GraphView::eHidden);
            mGraph->showInfos(false);
            mGraph->clearInfos();

        }
        /* ------------------------------------------------
         *  Events don't have std dev BUT we can visualize
         *  an overlay of all dates std dev instead.
         *  Possible curves, FOR ALL DATES :
         *  - Post Distrib Date i All Chains
         *  - Post Distrib Date i Chain j
         * ------------------------------------------------
         */
        else if (mShowVariableList.contains(eSigma)) {
            for (int i = 0; i < mEvent->mDates.size(); ++i) {
                mGraph->setCurveVisible("Post Distrib Date " + QString::number(i) + " All Chains", mShowAllChains);

                for (int j = 0; j < mShowChainList.size(); ++j)
                    mGraph->setCurveVisible("Post Distrib Date " + QString::number(i) + " Chain " + QString::number(j), mShowChainList.at(j));

            }
            mGraph->setTipXLab(tr("Sigma"));
            mGraph->setYAxisMode(GraphView::eHidden);

        }
        
        else if (mShowVariableList.contains(eVg)) {
            mGraph->setCurveVisible("Post Distrib All Chains", mShowAllChains);
            mGraph->setCurveVisible("HPD All Chains", mShowAllChains);

            for (int j = 0; j < mShowChainList.size(); ++j) {
                mGraph->setCurveVisible("Post Distrib Chain " + QString::number(j), mShowChainList.at(j));
            }
            
            mGraph->setTipXLab(tr("Std G"));
            mGraph->setYAxisMode(GraphView::eHidden);

        }
        mGraph->autoAdjustYScale(true);
    }
    /* -------------------- Second tab : History plots----------------------------
     *  Possible curves :
     *  - Trace i
     *  - Q1 i
     *  - Q2 i
     *  - Q3 i
     * ------------------------------------------------  */
    else if ((mCurrentTypeGraph == eTrace) &&
             (mShowVariableList.contains(eThetaEvent) || mShowVariableList.contains(eVg) )) {
             // We visualize only one chain (radio button)

            for (int i = 0; i < mShowChainList.size(); ++i) {
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

    /* ----------------------Third tab : Acceptance rate--------------------------
     *  Possible curves :
     *  - Accept i
     *  - Accept Target
     * ------------------------------------------------  */
    else if ((mCurrentTypeGraph == eAccept) &&
             (mShowVariableList.contains(eVg) ||
              ( mShowVariableList.contains(eThetaEvent) && ((mEvent->mTheta.mSamplerProposal == MHVariable::eMHAdaptGauss) || (mEvent->mTheta.mSamplerProposal == MHVariable::eFixe)))
               )) {
        mGraph->setCurveVisible("Accept Target", true);
        for (int i = 0; i < mShowChainList.size(); ++i)
            mGraph->setCurveVisible("Accept " + QString::number(i), mShowChainList.at(i));

        mGraph->setTipXLab(tr("Iteration"));
        mGraph->setTipYLab(tr("Rate"));

        mGraph->setYAxisMode(GraphView::eMinMax);
        mGraph->showInfos(false);
        mGraph->clearInfos();
        mGraph->autoAdjustYScale(false); // do  repaintGraph()
        mGraph->setRangeY(0, 100);
    }
    /* ----------------------fourth tab : Autocorrelation--------------------------
     *  Possible curves :
     *  - Correl i
     *  - Correl Limit Lower i
     *  - Correl Limit Upper i
     * ------------------------------------------------   */
    else if ( (mCurrentTypeGraph == eCorrel) &&
              (mShowVariableList.contains(eVg) ||
               (mShowVariableList.contains(eThetaEvent) && !isFixedBound)  )) {
            for (int i = 0; i < mShowChainList.size(); ++i) {
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
