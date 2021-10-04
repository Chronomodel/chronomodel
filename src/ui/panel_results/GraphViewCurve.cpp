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

#include "GraphViewCurve.h"
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

GraphViewCurve::GraphViewCurve(QWidget *parent):GraphViewResults(parent)
{
    setMainColor(Painting::borderDark);
    mGraph->setBackgroundColor(QColor(210, 210, 210));
}

GraphViewCurve::~GraphViewCurve()
{
    
}

void GraphViewCurve::setComposanteG(const PosteriorMeanGComposante& composante)
{
    mComposanteG = composante;
}

void GraphViewCurve::setComposanteGChains(const QList<PosteriorMeanGComposante>& composanteChains)
{
    mComposanteGChains = composanteChains;
}

void GraphViewCurve::setEvents(const QList<Event*>& events)
{
    mEvents = events;
}

void GraphViewCurve::paintEvent(QPaintEvent* e)
{
    GraphViewResults::paintEvent(e);
}

void GraphViewCurve::resizeEvent(QResizeEvent* )
{
    updateLayout();
}

void GraphViewCurve::generateCurves(const graph_t typeGraph, const QVector<variable_t>& variableList)
{
    GraphViewResults::generateCurves(typeGraph, variableList);
    
    mGraph->removeAllCurves();
    mGraph->removeAllZones();
    mGraph->clearInfos();
    mGraph->resetNothingMessage();
    mGraph->setOverArrow(GraphView::eNone);
    mGraph->setTipXLab("t");
    mGraph->setYAxisMode(GraphView::eMinMax);
    mGraph->autoAdjustYScale(true);
    mGraph->mLegendX = DateUtils::getAppSettingsFormatStr();
    mGraph->setFormatFunctX(nullptr);
    mGraph->setBackgroundColor(QColor(230, 230, 230));
    //mGraph->reserveCurves(5);
    
    if (mCurrentVariableList.contains(eG)) {
        GraphCurve curveEventsPoints;
        curveEventsPoints.mName = tr("Events Points");

        curveEventsPoints.mPen = QPen(Qt::black, 1, Qt::SolidLine);
        curveEventsPoints.mBrush = Qt::black;
        curveEventsPoints.mIsHisto = false;
        curveEventsPoints.mIsRectFromZero = false;
        curveEventsPoints.mIsRefPoints = true;

        for (auto& rf : mEventsPoints) {
            curveEventsPoints.mData.insert(rf.Xmean, rf.Ymean);
            curveEventsPoints.mDataErrorX.insert(rf.Xmean, rf.Xerr);
            curveEventsPoints.mDataErrorY.insert(rf.Xmean, rf.Yerr);
            curveEventsPoints.mDataColor.insert(rf.Xmean, rf.color);
        }

        GraphCurve curveDataPoints;
        curveDataPoints.mName = tr("Data Points");

        curveDataPoints.mPen = QPen(Qt::black, 1, Qt::SolidLine);
        curveDataPoints.mBrush = Qt::black;
        curveDataPoints.mIsHisto = false;
        curveDataPoints.mIsRectFromZero = false;
        curveDataPoints.mIsRefPoints = true;

        for (auto& rf : mDataPoints) {       
            curveDataPoints.mData.insert(rf.Xmean, rf.Ymean);
            curveDataPoints.mDataErrorX.insert(rf.Xmean, rf.Xerr);
            curveDataPoints.mDataErrorY.insert(rf.Xmean, rf.Yerr);
            curveDataPoints.mDataColor.insert(rf.Xmean, rf.color);
        }

        GraphCurve curveG;
        curveG.mName = tr("G");
        curveG.mPen = QPen(QColor(119, 95, 49), 1, Qt::SolidLine);
        curveG.mBrush = Qt::NoBrush;
        curveG.mIsHisto = false;
        curveG.mIsRectFromZero = false;

        GraphCurve curveGSup;
        curveGSup.mName = tr("G Sup");
        curveGSup.mPen = QPen(QColor(119, 95, 49), 1, Qt::CustomDashLine);
        curveGSup.mPen.setDashPattern(QList<qreal>{5, 5});
        curveGSup.mBrush = Qt::NoBrush;
        curveGSup.mIsHisto = false;
        curveGSup.mIsRectFromZero = false;

        GraphCurve curveGInf;
        curveGInf.mName = tr("G Inf");
        curveGInf.mPen = QPen(QColor(119, 95, 49), 1, Qt::CustomDashLine);
        curveGInf.mPen.setDashPattern(QList<qreal>{5, 5});
        curveGInf.mBrush = Qt::NoBrush;
        curveGInf.mIsHisto = false;
        curveGInf.mIsRectFromZero = false;
        QList<GraphCurve> curveGChains;
        for (int i = 0; i < mComposanteGChains.size(); ++i) {
            GraphCurve curveGChain;
            curveGChain.mName = QString("G Chain ") + QString::number(i);
            curveGChain.mPen = QPen(Painting::chainColors[i], 1, Qt::SolidLine);
            curveGChain.mBrush = Qt::NoBrush;
            curveGChain.mIsHisto = false;
            curveGChain.mIsRectFromZero = false;
            curveGChains.append(curveGChain);
        }

        double t;
        double step = mSettings.mStep;


        for (size_t idx = 0; idx < mComposanteG.vecG.size() ; ++idx) {

            t = DateUtils::convertToAppSettingsFormat(idx*step + mSettings.mTmin);
            curveG.mData.insert(t, mComposanteG.vecG.at(idx));
            // Enveloppe à 95%  https://en.wikipedia.org/wiki/1.96
            curveGSup.mData.insert(t, mComposanteG.vecG.at(idx) + 1.96 * sqrt(mComposanteG.vecVarG.at(idx)));
            curveGInf.mData.insert(t, mComposanteG.vecG.at(idx) - 1.96 * sqrt(mComposanteG.vecVarG.at(idx)));

            // Enveloppe à 68%
            //curveGSup.mData.insert(t, mComposanteG.vecG.at(idx) + 1. * mComposanteG.vecGErr.at(idx));
            //curveGInf.mData.insert(t, mComposanteG.vecG.at(idx) - 1. * mComposanteG.vecGErr.at(idx));

            for (int i = 0; i<curveGChains.size(); ++i) {
                curveGChains[i].mData.insert(t, mComposanteGChains.at(i).vecG.at(idx));
            }
        }

        mGraph->addCurve(curveG);
        mGraph->addCurve(curveGSup);
        mGraph->addCurve(curveGInf);

        mGraph->addCurve(curveEventsPoints);
        mGraph->addCurve(curveDataPoints);

        for (auto&& cGC: curveGChains) {
            mGraph->addCurve(cGC);
        }

    }

    else if (mCurrentVariableList.contains(eGP)) {
        GraphCurve curveGP;
        curveGP.mName = tr("G Prime");
        curveGP.mPen = QPen(QColor(154, 80, 225), 1, Qt::SolidLine);
        curveGP.mBrush = Qt::NoBrush;
        curveGP.mIsHisto = false;
        curveGP.mIsRectFromZero = false;
        double t;
        double step = mSettings.mStep;

        for (size_t idx = 0; idx < mComposanteG.vecG.size() ; ++idx) {
            t = DateUtils::convertToAppSettingsFormat(idx*step + mSettings.mTmin);
            curveGP.mData.insert(t, mComposanteG.vecGP.at(idx));
        }
        mGraph->addCurve(curveGP);

    } else if (mCurrentVariableList.contains(eGS)) {
        GraphCurve curveGS;
        curveGS.mName = tr("G Second");
        curveGS.mPen = QPen(QColor(236, 105, 64), 1, Qt::SolidLine);
        curveGS.mBrush = Qt::NoBrush;
        curveGS.mIsHisto = false;
        curveGS.mIsRectFromZero = false;
        double t;
        double step = mSettings.mStep;

        for (size_t idx = 0; idx < mComposanteG.vecG.size() ; ++idx) {
            t = DateUtils::convertToAppSettingsFormat(idx*step + mSettings.mTmin);
            curveGS.mData.insert(t, mComposanteG.vecGS.at(idx));
        }
        mGraph->addCurve(curveGS);
    }

    mGraph->setTipXLab(tr("t"));
    mGraph->setTipYLab("Y");

}

void GraphViewCurve::updateCurvesToShowForG(bool showAllChains, QList<bool> showChainList, const QVector<variable_t>& showVariableList)
{
    // From GraphViewResults::updateCurvesToShow
    mShowAllChains = showAllChains;
    mShowChainList = showChainList;
    
    const bool showG = showVariableList.contains(eG);
    const bool showGError = showVariableList.contains(eGError);
    const bool showEventsPoints = showVariableList.contains(eGEventsPts);
    const bool showDataPoints = showVariableList.contains(eGDatesPts);
    const bool showGP = showVariableList.contains(eGP);
    const bool showGS = showVariableList.contains(eGS);
    
    mGraph->setCurveVisible("G", mShowAllChains && showG);
    mGraph->setCurveVisible("G Sup", mShowAllChains && showGError);
    mGraph->setCurveVisible("G Inf", mShowAllChains && showGError);
    mGraph->setCurveVisible("Events Points", showEventsPoints);
    mGraph->setCurveVisible("Data Points", showDataPoints);
    mGraph->setCurveVisible("G Prime", mShowAllChains && showGP);
    mGraph->setCurveVisible("G Second", mShowAllChains && showGS);
    
    for (int i = 0; i < mShowChainList.size(); ++i) {
        mGraph->setCurveVisible(QString("G Chain ") + QString::number(i), mShowChainList.at(i) && showG);
    }
}
