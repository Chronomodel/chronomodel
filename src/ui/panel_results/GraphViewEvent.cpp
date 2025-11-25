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

#include "GraphViewEvent.h"
#include "GraphView.h"
#include "Event.h"
#include "Bound.h"

#include "ModelUtilities.h"
#include "Painting.h"

GraphViewEvent::GraphViewEvent(QWidget *parent):
    GraphViewResults(parent),
    mEvent(nullptr)
{
    setMainColor(QColor(100, 100, 100));
    mGraph->setBackgroundColor(QColor(230, 230, 230));
}

GraphViewEvent::~GraphViewEvent()
{
}

void GraphViewEvent::setEvent(std::shared_ptr<Event> event)
{
    Q_ASSERT(event);
    mEvent = event;
    setItemColor(mEvent->mColor);
    //update();
}


void GraphViewEvent::generateCurves(const graph_t typeGraph,const QList<variable_t> &variableList)
{
    GraphViewResults::generateCurves(typeGraph, variableList);

    /* ------------------------------------------------
     *  Reset the graph object settings
     * ------------------------------------------------
     */
    graph_reset();

    QPen defaultPen;
    defaultPen.setWidthF(1);
    defaultPen.setStyle(Qt::SolidLine);
    // set default title
    mTitle = ((mEvent->type()==Event::eBound) ? tr("Bound") : tr("Event")) + " : " + mEvent->getQStringName();

    const QColor color = mEvent->mColor;

    QString resultsHTML = tr("Nothing to Display");

    if (mCurrentVariableList.contains(eS02)) {
        resultsHTML = ModelUtilities::EventS02ResultsHTML(mEvent);

    } else if (mCurrentVariableList.contains(eVg)) {
        resultsHTML = ModelUtilities::VgResultsHTML(mEvent);

    } else if (mCurrentVariableList.contains(eThetaEvent)) {
        resultsHTML = ModelUtilities::eventResultsHTML(mEvent, false);
    }

    setNumericalResults(resultsHTML);

    bool isFixedBound = false;
    Bound* bound = nullptr;
    if (mEvent->type() == Event::eBound) {
        bound = dynamic_cast<Bound*>(mEvent.get());
        isFixedBound = (bound != nullptr);
    }
    
    // --------------------------------------------------------------------
    //  The graph name depends on the currently displayed variable
    // --------------------------------------------------------------------
    if (mCurrentVariableList.contains(eThetaEvent)) {
        mTitle = ((mEvent->type()==Event::eBound) ? tr("Bound") : tr("Event")) + " : " + mEvent->getQStringName();

    } else if (mCurrentVariableList.contains(eSigma)) {
        if (typeGraph == ePostDistrib)
            mTitle = ((mEvent->type() == Event::eBound) ? tr("Bound") : tr("Std Compilation")) + " : " + mEvent->getQStringName();
        else
            mTitle = ((mEvent->type()==Event::eBound) ? tr("Bound") : tr("Event")) + " : " + mEvent->getQStringName();

    } else if (mCurrentVariableList.contains(eS02) && mEvent->mS02Theta.mSamplerProposal != MHVariable::eFixe) {
        mTitle = tr("Event Shrinkage") + " : " + mEvent->getQStringName();

    } else if (mCurrentVariableList.contains(eVg)) {
        mTitle = tr("Std gi") + " : " + mEvent->getQStringName();
    }

    // ------------------------------------------------
    //  First tab : Posterior distrib
    // ------------------------------------------------
    if (typeGraph == ePostDistrib) {

        //mGraph->mLegendX = DateUtils::getAppSettingsFormatStr();
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
            GraphViewResults::graph_density();
            if (isFixedBound) {
                GraphCurve curveLineBound;
                curveLineBound.mName = "Post Distrib All Chains";
                curveLineBound.mPen.setColor(color);
                curveLineBound.mBrush.setStyle(Qt::NoBrush);
                curveLineBound.mType = GraphCurve::eHorizontalSections;
                qreal tLower = bound->formatedFixedValue();
                qreal tUpper = tLower;
                curveLineBound.mSections.push_back(qMakePair(tLower,tUpper));
                mGraph->add_curve(curveLineBound);

                // generate theorical curves
                for (size_t i = 0; i < mChains.size(); ++i) {
                    curveLineBound.mName = "Post Distrib Chain " + QString::number(i);
                    curveLineBound.mPen.setColor(Painting::chainColors.at(i));
                    curveLineBound.mBrush.setStyle(Qt::NoBrush);
                    mGraph->add_curve(curveLineBound);
                }

            } else {


                /* ------------------------------------
                 *  Post Distrib All Chains
                 * ------------------------------------
                 */
                const GraphCurve &curvePostDistrib = densityCurve(mEvent->mTheta.mFormatedHisto,
                                                                       "Post Distrib All Chains",
                                                                       color);
                mGraph->add_curve(curvePostDistrib);

                // HPD All Chains
                const GraphCurve &curveHPD = HPDCurve(mEvent->mTheta.mFormatedHPD,
                                                       "HPD All Chains",
                                                       color);
                mGraph->add_curve(curveHPD);

                /* ------------------------------------
                 *  Post Distrib Chain i
                 * ------------------------------------
                 */
                if (!mEvent->mTheta.mChainsHistos.empty())
                    for (size_t i = 0; i < mChains.size(); ++i) {
                        const GraphCurve &curvePostDistribChain = densityCurve(mEvent->mTheta.mChainsHistos[i],
                                                                                "Post Distrib Chain " + QString::number(i),
                                                                                Painting::chainColors.at(i),
                                                                                Qt::SolidLine,
                                                                                Qt::NoBrush);
                        mGraph->add_curve(curvePostDistribChain);
                    }

                /* ------------------------------------
                 *  Theta Credibility
                 * ------------------------------------
                 */
                const GraphCurve &curveCred = topLineSection(mEvent->mTheta.mFormatedCredibility,
                                                                "Credibility All Chains",
                                                                color);
                mGraph->add_curve(curveCred);
            }

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
            graph_density();
            mGraph->removeAllCurves(); // delete default zones made by graph_density()
            mGraph->setOverArrow(GraphView::eNone);
            mGraph->mLegendX = "";

            mGraph->setBackgroundColor(QColor(230, 230, 230));

            int i = 0;
            for (auto&& date : mEvent->mDates) {
                GraphCurve curve = densityCurve(date.mSigmaTi.fullHisto(),
                                                        "Post Distrib Date " + QString::number(i) + " All Chains",
                                                        color);
                curve.mVisible = true;
                mGraph->add_curve(curve);
                if (!date.mSigmaTi.mChainsHistos.empty())
                    for (size_t j=0; j<mChains.size(); ++j) {
                        const GraphCurve &curveChain = densityCurve(date.mSigmaTi.histoForChain(j),
                                                                     "Post Distrib Date " + QString::number(i) + " Chain " + QString::number(j),
                                                                     Painting::chainColors.at(j));
                        mGraph->add_curve(curveChain);
                    }

                ++i;
            }
        }

        else if (mCurrentVariableList.contains(eS02) && mEvent->mS02Theta.mSamplerProposal != MHVariable::eFixe) {
            GraphViewResults::graph_density();
            mGraph->removeAllCurves(); // delete default zones made by graph_density()

            mGraph->setOverArrow(GraphView::eNone);
            mGraph->mLegendX = "";
            mGraph->setBackgroundColor(QColor(230, 230, 230));

            GraphCurve curve = densityCurve(mEvent->mS02Theta.fullHisto(), "Post Distrib All Chains", color);
            curve.mVisible = true;
            mGraph->add_curve(curve);

            if (!mEvent->mS02Theta.mChainsHistos.empty()) {
                for (size_t j = 0; j<mChains.size(); ++j) {
                    const GraphCurve &curveChain = densityCurve(mEvent->mS02Theta.histoForChain(j), "Post Distrib Chain " + QString::number(j), Painting::chainColors.at(j));
                    mGraph->add_curve(curveChain);
                }
            }

            // HPD All Chains
            const GraphCurve &curveHPD = HPDCurve(mEvent->mS02Theta.mFormatedHPD, "HPD All Chains", color);
            mGraph->add_curve(curveHPD);

            /* ------------------------------------
             *  Theta Credibility
             * ------------------------------------
             */
            const GraphCurve &curveCred = topLineSection(mEvent->mS02Theta.mFormatedCredibility,
                                                         "Credibility All Chains",
                                                         color);
            mGraph->add_curve(curveCred);
        }

        else if (mCurrentVariableList.contains(eVg)) {
            GraphViewResults::graph_density();
            mGraph->removeAllCurves(); // delete default zones made by graph_density()

            mGraph->setOverArrow(GraphView::eNone);
            mGraph->mLegendX = "";
            mGraph->setBackgroundColor(QColor(230, 230, 230));

            GraphCurve curve = densityCurve(mEvent->mVg.fullHisto(), "Post Distrib All Chains", color);
            curve.mVisible = true;
            mGraph->add_curve(curve);
            
            if (!mEvent->mVg.mChainsHistos.empty()) {
                for (size_t j = 0; j<mChains.size(); ++j) {
                    const GraphCurve &curveChain = densityCurve(mEvent->mVg.histoForChain(j), "Post Distrib Chain " + QString::number(j), Painting::chainColors.at(j));
                    mGraph->add_curve(curveChain);
                }
            }

            // HPD All Chains
            const GraphCurve &curveHPD = HPDCurve(mEvent->mVg.mFormatedHPD, "HPD All Chains", color);
            mGraph->add_curve(curveHPD);

            /* ------------------------------------
             *  Vg Credibility
             * ------------------------------------
             */
            const GraphCurve &curveCred = topLineSection(mEvent->mVg.mFormatedCredibility,
                                                            "Credibility All Chains",
                                                            color);
            mGraph->add_curve(curveCred);
        }


    }
    // ----------------------------------------------------------------------
    //  Trace : generate Trace, Q1, Q2, Q3 for all chains
    // ----------------------------------------------------------------------
    else if (typeGraph == eTrace) {
        GraphViewResults::graph_trace();

        if (mCurrentVariableList.contains(eThetaEvent) && mEvent->mTheta.mSamplerProposal != MHVariable::eFixe) {
            generateTraceCurves(mChains, &(mEvent->mTheta));

        } else if (mCurrentVariableList.contains(eS02) && mEvent->mS02Theta.mSamplerProposal != MHVariable::eFixe) {
            mTitle = tr("Log10(Event Shrinkage) : %1").arg(mEvent->getQStringName());
            generateLogTraceCurves(mChains, &(mEvent->mS02Theta));

        } else if (mCurrentVariableList.contains(eVg) && mEvent->mVg.mSamplerProposal != MHVariable::eFixe) {
            mTitle = tr("Log10(Std gi) : %1").arg(mEvent->getQStringName());
            generateLogTraceCurves(mChains, &(mEvent->mVg));
        }
    }
    // ----------------------------------------------------------------------
    //  Acceptance rate : generate acceptance rate and accept target curves
    // ----------------------------------------------------------------------
    else if (typeGraph == eAccept)  {
        GraphViewResults::graph_acceptation();

        if (mCurrentVariableList.contains(eThetaEvent) ) {
            if (mEvent->mTheta.mSamplerProposal == MHVariable::eMHAdaptGauss) {
                generateAcceptCurves(mChains, &(mEvent->mTheta));
            } else {
                mGraph->setNothingMessage(tr("100 %"));
            }

        } else if (mCurrentVariableList.contains(eS02) && mEvent->mS02Theta.mSamplerProposal != MHVariable::eFixe) {
            generateAcceptCurves(mChains, &(mEvent->mS02Theta));

        } else if (mCurrentVariableList.contains(eVg) && mEvent->mVg.mSamplerProposal != MHVariable::eFixe) {
            generateAcceptCurves(mChains, &(mEvent->mVg));
        }

    }
    // ----------------------------------------------------------------------
    //  Autocorrelation : generate correlation with lower and upper limits
    // ----------------------------------------------------------------------
    else if ( typeGraph == eCorrel ) {
        GraphViewResults::graph_correlation();

        if (mCurrentVariableList.contains(eS02) && mEvent->mS02Theta.mSamplerProposal!= MHVariable::eFixe) {
            generateCorrelCurves(mChains, &(mEvent->mS02Theta));

        } else if (mCurrentVariableList.contains(eVg) && mEvent->mVg.mSamplerProposal!= MHVariable::eFixe) {
            generateCorrelCurves(mChains, &(mEvent->mVg));

        } else if (mCurrentVariableList.contains(eThetaEvent) && mEvent->mTheta.mSamplerProposal!= MHVariable::eFixe) {
            generateCorrelCurves(mChains, &(mEvent->mTheta));
        }

    }


}

void GraphViewEvent::updateCurvesToShow(bool showAllChains, const QList<bool>& showChainList, const QList<variable_t>& showVariableList)
{
    Q_ASSERT(mEvent);
    GraphViewResults::updateCurvesToShow(showAllChains, showChainList, showVariableList);
    QStringList curvesToShow;

    /* --------------------First tab : Posterior distrib----------------------------
     * ------------------------------------------------*/
    if (mCurrentTypeGraph == ePostDistrib) {

        /* ------------------------------------------------
         *  Possible curves :
         *  - Post Distrib All Chains
         *  - HPD All Chains
         *  - Credibility All Chains
         *  - Post Distrib Chain i
         * ------------------------------------------------
         */
        if (mCurrentVariableList.contains(eThetaEvent)) {

            //QStringList curvesToShow;

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

            //mGraph->setCurveVisible(curvesToShow, true);

        }
        /* ------------------------------------------------
         *  Events don't have std dev BUT we can visualize
         *  an overlay of all dates std dev instead.
         *  Possible curves, FOR ALL DATES :
         *  - Post Distrib Date i All Chains
         *  - Post Distrib Date i Chain j
         * ------------------------------------------------
         */
        else if (mCurrentVariableList.contains(eSigma)) {
                //QStringList curvesToShow;
                for (int i = 0; i < (int)mEvent->mDates.size(); ++i) {
                    if (mShowAllChains) {
                        curvesToShow << QString("Post Distrib Date %1 All Chains").arg(i);

                        if (mShowVariableList.contains(eCredibility))
                            curvesToShow << "Credibility All Chains";
                    }
                    for (int j = 0; j < mShowChainList.size(); ++j)
                        if (mShowChainList.at(j))
                            curvesToShow << QString("Post Distrib Date %1 Chain %2").arg(i).arg(j);

                }
                //mGraph->setCurveVisible(curvesToShow, true);
                mGraph->setTipXLab(tr("Sigma"));

        }
        
        else if (mCurrentVariableList.contains(eS02) ) {
            //QStringList curvesToShow;
            if (mShowAllChains) {
                curvesToShow << "Post Distrib All Chains" << "HPD All Chains";
                if (mShowVariableList.contains(eCredibility))
                    curvesToShow << "Credibility All Chains";

            }
            for (int j = 0; j < mShowChainList.size(); ++j) {
                if (mShowChainList.at(j))
                    curvesToShow << QString("Post Distrib Chain %1").arg(j);

            }
            //mGraph->setCurveVisible(curvesToShow, true);
            mGraph->setTipXLab(tr("Event Shrinkage"));


        }
        else if (mCurrentVariableList.contains(eVg)) {

                //QStringList curvesToShow;
                if (mShowAllChains) {
                    curvesToShow << "Post Distrib All Chains" << "HPD All Chains";
                    if (mShowVariableList.contains(eCredibility))
                        curvesToShow << "Credibility All Chains";
                }
                for (int j = 0; j < mShowChainList.size(); ++j) {
                    if (mShowChainList.at(j))
                        curvesToShow << QString("Post Distrib Chain %1").arg(j);
                }
                //mGraph->setCurveVisible(curvesToShow, true);
                mGraph->setTipXLab(tr("Std gi"));


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
    else if ( (mCurrentTypeGraph == eTrace) &&
              ( (mCurrentVariableList.contains(eVg) && mEvent->mVg.mSamplerProposal != MHVariable::eFixe ) ||
              (mCurrentVariableList.contains(eS02) && mEvent->mS02Theta.mSamplerProposal != MHVariable::eFixe ) ||
                ( mCurrentVariableList.contains(eThetaEvent) && mEvent->mTheta.mSamplerProposal != MHVariable::eFixe) )) {
             // We visualize only one chain (radio button)

            //QStringList curvesToShow;
            for (int j = 0; j < mShowChainList.size(); ++j) {
                if (mShowChainList.at(j)) {
                    curvesToShow << QString("Trace %1").arg(j);
                    curvesToShow << QString("Q1 %1").arg(j);
                    curvesToShow << QString("Q2 %1").arg(j);
                    curvesToShow << QString("Q3 %1").arg(j);
                }
            }
            //mGraph->setCurveVisible(curvesToShow, true);
    }

    /* ----------------------Third tab : Acceptance rate--------------------------
     *  Possible curves :
     *  - Accept i
     *  - Accept Target
     * ------------------------------------------------  */
    else if ( (mCurrentTypeGraph == eAccept) &&
              ( (mCurrentVariableList.contains(eVg) && mEvent->mVg.mSamplerProposal != MHVariable::eFixe ) ||
              (mCurrentVariableList.contains(eS02) && mEvent->mS02Theta.mSamplerProposal != MHVariable::eFixe ) ||
              ( mCurrentVariableList.contains(eThetaEvent) && mEvent->mTheta.mSamplerProposal == MHVariable::eMHAdaptGauss))) {

               // QStringList curvesToShow;
                curvesToShow << "Accept Target";
                for (int j = 0; j < mShowChainList.size(); ++j) {
                    if (mShowChainList.at(j)) {
                        curvesToShow << QString("Accept %1").arg(j);
                    }
                }
                //mGraph->setCurveVisible(curvesToShow, true);

    }
    /* ----------------------fourth tab : Autocorrelation--------------------------
     *  Possible curves :
     *  - Correl i
     *  - Correl Limit Lower i
     *  - Correl Limit Upper i
     * ------------------------------------------------   */
    else if ( (mCurrentTypeGraph == eCorrel) &&
              ( (mCurrentVariableList.contains(eVg) && mEvent->mVg.mSamplerProposal != MHVariable::eFixe ) ||
                (mCurrentVariableList.contains(eS02) && mEvent->mS02Theta.mSamplerProposal != MHVariable::eFixe ) ||
                ( mCurrentVariableList.contains(eThetaEvent) && mEvent->mTheta.mSamplerProposal != MHVariable::eFixe))) {

                //QStringList curvesToShow;
                for (int j = 0; j < mShowChainList.size(); ++j) {
                    if (mShowChainList.at(j)) {
                        curvesToShow << QString("Correl %1").arg(j);
                        curvesToShow << QString("Correl Limit Lower %1").arg(j);
                        curvesToShow << QString("Correl Limit Upper %1").arg(j);
                    }
                }
                //mGraph->setCurveVisible(curvesToShow, true);

    }


    mGraph->setCurveVisible(curvesToShow, true);
    repaint();
}
