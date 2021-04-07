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
#include "EventKnown.h"
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
    setItemTitle(eventTitle);
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

void GraphViewEvent::generateCurves(TypeGraph typeGraph, Variable variable)
{
    Q_ASSERT(mEvent);
    
    GraphViewResults::generateCurves(typeGraph, variable);

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
    QString resultsText = ModelUtilities::eventResultsText(mEvent, false);
    QString resultsHTML = ModelUtilities::eventResultsHTML(mEvent, false);
    setNumericalResults(resultsHTML, resultsText);

    bool isFixedBound = false;
    EventKnown* bound = nullptr;
    if (mEvent->type() == Event::eKnown) {
        bound = dynamic_cast<EventKnown*>(mEvent);
        isFixedBound = (bound != nullptr);
    }
    
    // --------------------------------------------------------------------
    //  The graph name depends on the currently displayed variable
    // --------------------------------------------------------------------
    if (variable == eTheta) {
        mTitle = ((mEvent->type()==Event::eKnown) ? tr("Bound") : tr("Event")) + " : " + mEvent->mName;

    } else if (variable == eSigma) {
        mTitle = ((mEvent->type() == Event::eKnown) ? tr("Bound") : tr("Std Compilation")) + " : " + mEvent->mName;

    } else if (variable == eVG) {
        mTitle = tr("Variance G") + " : " + mEvent->mName;
    }

    // ------------------------------------------------
    //  First tab : Posterior distrib
    // ------------------------------------------------
    if (typeGraph == ePostDistrib) {
        mGraph->mLegendX = DateUtils::getAppSettingsFormatStr();
        mGraph->setBackgroundColor(QColor(230, 230, 230));
        
        mTitle = ((mEvent->type()==Event::eKnown) ? tr("Bound : %1").arg(mEvent->mName) : tr("Event : %1").arg(mEvent->mName));

        /* ------------------------------------------------
         *  Possible curves :
         *  - Post Distrib All Chains
         *  - HPD All Chains
         *  - Credibility All Chains
         *  - Post Distrib Chain i
         * ------------------------------------------------
         */
        if (variable == eTheta) {
            mGraph->setOverArrow(GraphView::eBothOverflow);
            if (isFixedBound) {
                GraphCurve curveLineBound;
                curveLineBound.mName = "Post Distrib All Chains";
                curveLineBound.mPen.setColor(color);
                curveLineBound.mIsHorizontalSections = true;
                qreal tLower = bound->formatedFixedValue();
                qreal tUpper = tLower;
                curveLineBound.mSections.append(qMakePair(tLower,tUpper));
                mGraph->addCurve(curveLineBound);

                // generate theorical curves
                for (int i=0; i<mChains.size(); ++i) {
                   // GraphCurve curveLineBound;
                    curveLineBound.mName = "Post Distrib Chain " + QString::number(i);
                    curveLineBound.mPen.setColor(Painting::chainColors.at(i));

                    mGraph->addCurve(curveLineBound);
                }

            }


            // ------------------------------------
            //  HPD All Chains
            // ------------------------------------
            if (!isFixedBound) {
                /* ------------------------------------
                 *  Post Distrib All Chains
                 * ------------------------------------
                 */
                GraphCurve curvePostDistrib;
                curvePostDistrib.mName = "Post Distrib All Chains";
                curvePostDistrib.mPen.setColor(color);

                curvePostDistrib = generateDensityCurve(mEvent->mTheta.fullHisto(),
                                                                       "Post Distrib All Chains",
                                                                       color);
                mGraph->addCurve(curvePostDistrib);

                // HPD All Chains
                GraphCurve curveHPD = generateHPDCurve(mEvent->mTheta.mHPD,
                                                       "HPD All Chains",
                                                       color);
                mGraph->addCurve(curveHPD);

                /* ------------------------------------
                 *  Post Distrib Chain i
                 * ------------------------------------
                 */
                if (!mEvent->mTheta.mChainsHistos.isEmpty())
                    for (int i = 0; i < mChains.size(); ++i) {
                        GraphCurve curvePostDistribChain = generateDensityCurve(mEvent->mTheta.histoForChain(i),
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
                GraphCurve curveCred = generateSectionCurve(mEvent->mTheta.mCredibility,
                                                                "Credibility All Chains",
                                                                color);
                mGraph->addCurve(curveCred);
            }
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
        }


        /* ------------------------------------------------
         *  Events don't have std dev BUT we can visualize
         *  an overlay of all dates std dev instead.
         *  Possible curves, FOR ALL DATES :
         *  - Sigma Date i All Chains
         *  - Sigma Date i Chain j
         * ------------------------------------------------
         */
        else if (variable == eSigma) {
            mGraph->setOverArrow(GraphView::eNone);
            mGraph->mLegendX = "";

            mGraph->setBackgroundColor(QColor(230, 230, 230));

            int i = 0;
            for (auto&& date : mEvent->mDates) {
                GraphCurve curve = generateDensityCurve(date.mSigma.fullHisto(),
                                                        "Sigma Date " + QString::number(i) + " All Chains",
                                                        color);

                mGraph->addCurve(curve);
                if (!date.mSigma.mChainsHistos.isEmpty())
                    for (int j=0; j<mChains.size(); ++j) {
                        GraphCurve curveChain = generateDensityCurve(date.mSigma.histoForChain(j),
                                                                     "Sigma Date " + QString::number(i) + " Chain " + QString::number(j),
                                                                     Painting::chainColors.at(j));
                        mGraph->addCurve(curveChain);
                    }
                ++i;
            }
        }
        
        else if (variable == eVG) {
            mGraph->setOverArrow(GraphView::eNone);
            mGraph->mLegendX = "";
            mGraph->setBackgroundColor(QColor(230, 230, 230));

            GraphCurve curve = generateDensityCurve(mEvent->mVG.fullHisto(), "Variance G All Chains", color);
            // qDebug() << curve.mData;
            mGraph->addCurve(curve);
            
            if (!mEvent->mVG.mChainsHistos.isEmpty()) {
                for (int j = 0; j<mChains.size(); ++j) {
                    GraphCurve curveChain = generateDensityCurve(mEvent->mVG.histoForChain(j), "Variance G Chain " + QString::number(j), color);
                    mGraph->addCurve(curveChain);
                }
            }
        }
    }
    // ----------------------------------------------------------------------
    //  Trace : generate Trace, Q1, Q2, Q3 for all chains
    // ----------------------------------------------------------------------
    else if (typeGraph == eTrace) {
        mGraph->mLegendX = "Iterations";
        
        if (variable == eTheta) {
            generateTraceCurves(mChains, &(mEvent->mTheta));

        } else if (variable == eVG) {
            generateTraceCurves(mChains, &(mEvent->mVG));
        }
    }
    // ----------------------------------------------------------------------
    //  Acceptance rate : generate acceptance rate and accept target curves
    // ----------------------------------------------------------------------
    else if (typeGraph == eAccept)  {
        mGraph->mLegendX = "Iterations";
        
        if (variable == eTheta && (mEvent->mMethod == Event::eMHAdaptGauss || mEvent->mMethod == Event::eFixe)) {
            generateAcceptCurves(mChains, &(mEvent->mTheta));
            mGraph->addCurve(generateHorizontalLine(44, "Accept Target", QColor(180, 10, 20), Qt::DashLine));

        } else if (variable == eVG) {
            generateAcceptCurves(mChains, &(mEvent->mVG));
            mGraph->addCurve(generateHorizontalLine(44, "Accept Target", QColor(180, 10, 20), Qt::DashLine));
        }
    }
    // ----------------------------------------------------------------------
    //  Autocorrelation : generate correlation with lower and upper limits
    // ----------------------------------------------------------------------
    else if ((typeGraph == eCorrel) && (variable == eTheta) && (!isFixedBound)) {
        mGraph->mLegendX = "";
        generateCorrelCurves(mChains, &(mEvent->mTheta));
        mGraph->setXScaleDivision(10, 10);

    } else {
        mTitle = ((mEvent->type()==Event::eKnown) ? tr("Bound : %1").arg(mEvent->mName) : tr("Event : %1").arg(mEvent->mName));
        mGraph->resetNothingMessage();
    }

}

void GraphViewEvent::updateCurvesToShow(bool showAllChains, const QList<bool>& showChainList, bool showCredibility, bool showCalib, bool showWiggle)
{
    Q_ASSERT(mEvent);
    GraphViewResults::updateCurvesToShow(showAllChains, showChainList, showCredibility, showCalib, showWiggle);

    bool isFixedBound = false;

  //  EventKnown* bound = nullptr;
    if (mEvent->type() == Event::eKnown) {
  //      bound = dynamic_cast<EventKnown*>(mEvent);
        isFixedBound = true;

    }

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
        if (mCurrentVariable == eTheta) {
            mGraph->setCurveVisible("Post Distrib All Chains", mShowAllChains);
            mGraph->setCurveVisible("HPD All Chains", mShowAllChains);
            mGraph->setCurveVisible("Credibility All Chains", mShowCredibility && mShowAllChains);

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
         *  - Sigma Date i All Chains
         *  - Sigma Date i Chain j
         * ------------------------------------------------
         */
        else if (mCurrentVariable == eSigma) {
            for (int i = 0; i < mEvent->mDates.size(); ++i) {
                mGraph->setCurveVisible("Sigma Date " + QString::number(i) + " All Chains", mShowAllChains);

                for (int j = 0; j < mShowChainList.size(); ++j)
                    mGraph->setCurveVisible("Sigma Date " + QString::number(i) + " Chain " + QString::number(j), mShowChainList.at(j));

            }
            mGraph->setTipXLab(tr("Sigma"));
            mGraph->setYAxisMode(GraphView::eHidden);

        }
        
        else if (mCurrentVariable == eVG) {
            mGraph->setCurveVisible("Variance G All Chains", mShowAllChains);

            for (int j = 0; j < mShowChainList.size(); ++j) {
                mGraph->setCurveVisible("Variance G Chain " + QString::number(j), mShowChainList.at(j));
            }
            
            mGraph->setTipXLab(tr("Variance G"));
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
    else if (mCurrentTypeGraph == eTrace && mCurrentVariable == eTheta) {
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
    else if ((mCurrentTypeGraph == eAccept) && (mCurrentVariable == eTheta) && ((mEvent->mMethod == Event::eMHAdaptGauss) || (mEvent->mMethod == Event::eFixe))) {
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
    else if (mCurrentTypeGraph == eCorrel && mCurrentVariable == eTheta && !isFixedBound) {
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

    update();
}
