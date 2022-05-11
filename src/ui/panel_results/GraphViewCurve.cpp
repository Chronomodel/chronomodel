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
    
    double t;

    // We use the parameter saved with the map
    const double step = (mComposanteG.mapG.maxX() - mComposanteG.mapG.minX()) / (mComposanteG.mapG._column -1);
    const double tmin = mComposanteG.mapG.minX();

    if (mCurrentVariableList.contains(eG)) {
        GraphCurve curveEventsPoints;
        curveEventsPoints.mName = tr("Events Points");

        curveEventsPoints.mPen = QPen(Qt::black, 1, Qt::SolidLine);
        curveEventsPoints.mBrush = Qt::black;
        curveEventsPoints.mIsRectFromZero = false;

        curveEventsPoints.mType = GraphCurve::eRefPoints;

        CurveRefPts ref;
        for (auto& ePts : mEventsPoints) {
            ref.Xmin = DateUtils::convertToAppSettingsFormat(ePts.Xmin);
            ref.Xmax = DateUtils::convertToAppSettingsFormat(ePts.Xmax);
            if (ref.Xmin > ref.Xmax)
                std::swap(ref.Xmin, ref.Xmax);

            ref.Ymin = ePts.Ymin;
            ref.Ymax = ePts.Ymax;
            ref.type = CurveRefPts::eLine;
            ref.color = ePts.color;
            curveEventsPoints.mRefPoints.push_back(ref);
        }

        GraphCurve curveDataPoints;
        curveDataPoints.mName = tr("Data Points");

        curveDataPoints.mPen = QPen(Qt::black, 1, Qt::SolidLine);
        curveDataPoints.mBrush = Qt::black;
        curveDataPoints.mIsRectFromZero = false;

        curveDataPoints.mType = GraphCurve::eRefPoints;

        for (auto& dPts : mDataPoints) {
            ref.Xmin = DateUtils::convertToAppSettingsFormat(dPts.Xmin);
            ref.Xmax = DateUtils::convertToAppSettingsFormat(dPts.Xmax);
            if (ref.Xmin > ref.Xmax)
                std::swap(ref.Xmin, ref.Xmax);

            ref.Ymin = dPts.Ymin;
            ref.Ymax = dPts.Ymax;
            ref.type = CurveRefPts::eCross;
            ref.color = dPts.color;
            curveDataPoints.mRefPoints.push_back(ref);

        }

        GraphCurve curveG;
        curveG.mName = tr("G");
        curveG.mPen = QPen(QColor(119, 95, 49), 1, Qt::SolidLine);
        curveG.mBrush = Qt::NoBrush;
        curveG.mIsRectFromZero = false;

        GraphCurve curveGInf;
        curveGInf.mName = tr("G Inf");
        curveGInf.mPen = QPen(QColor(119, 95, 49), 1, Qt::CustomDashLine);
        curveGInf.mPen.setDashPattern(QList<qreal>{5, 5});
        curveGInf.mBrush = Qt::NoBrush;
        curveGInf.mIsRectFromZero = false;

        GraphCurve curveGSup;
        curveGSup.mName = tr("G Sup");
        curveGSup.mPen = QPen(QColor(119, 95, 49), 1, Qt::CustomDashLine);
        curveGSup.mPen.setDashPattern(QList<qreal>{5, 5});
        curveGSup.mBrush = Qt::NoBrush;
        curveGSup.mIsRectFromZero = false;

        GraphCurve curveGEnv;
      /*  curveGEnv.mName = tr("G Env");
        curveGEnv.mPen = QPen(QColor(119, 95, 49), 1, Qt::CustomDashLine);
        curveGEnv.mPen.setDashPattern(QList<qreal>{5, 5});
        curveGEnv.mBrush = QColor(119, 95, 49);
        curveGEnv.mIsRectFromZero = false; */
        curveGEnv.mType = GraphCurve::eShapeData;

        GraphCurve curveMap;
        curveMap.mName = tr("Map");
        //curveMap.mPen = QPen(QColor(119, 95, 49), 1, Qt::SolidLine);
        curveMap.mPen = QPen(Qt::black, 1, Qt::SolidLine);
        curveMap.mBrush = Qt::NoBrush;
        curveMap.mIsRectFromZero = false;

        curveMap.mType = GraphCurve::eCurveMap;
        curveMap.mMap = mComposanteG.mapG;
        const double tminFormated = DateUtils::convertToAppSettingsFormat(mComposanteG.mapG.minX());
        const double tmaxFormated = DateUtils::convertToAppSettingsFormat(mComposanteG.mapG.maxX());

        if (tmaxFormated > tminFormated) {
            curveMap.mMap.setRangeX(tminFormated, tmaxFormated);

        } else {
            curveMap.mMap.setRangeX(tmaxFormated, tminFormated);
            // we must reflect the map
           // CurveMap displayMap (0, 0);
            CurveMap displayMap (curveMap.mMap._row, curveMap.mMap._column);

            int c  = curveMap.mMap._column-1;
           // displayMap.data = std::valarray<double>(curveMap.mMap._row * curveMap.mMap._column);
            unsigned i = 0 ;
            while ( c >= 0) {
                for (unsigned r = 0; r < curveMap.mMap._row ; r++) {
                    displayMap.data[i++] = mComposanteG.mapG.at(c, r);
                    //displayMap.data.push_back(mComposanteG.mapG.at(c, r));
                }
                c--;
            }

            curveMap.mMap.data = std::move(displayMap.data);
        }


        QList<GraphCurve> curveGChains;
        QList<GraphCurve> curveGInfChains;
        QList<GraphCurve> curveGSupChains;
        QList<GraphCurve> curveMapChains;

        for (unsigned i = 0; i < mComposanteGChains.size(); ++i) {
            GraphCurve curveGChain;
            curveGChain.mName = QString("G Chain ") + QString::number(i);
            curveGChain.mPen = QPen(Painting::chainColors[i], 1, Qt::SolidLine);
            curveGChain.mBrush = Qt::NoBrush;
            curveGChain.mIsRectFromZero = false;

            curveGChains.append(curveGChain);

            GraphCurve curveGInfChain;
            curveGInfChain.mName = QString("G Inf Chain ") + QString::number(i);
            curveGInfChain.mPen = QPen(Painting::chainColors[i], 1, Qt::CustomDashLine);
            curveGInfChain.mPen.setDashPattern(QList<qreal>{5, 5});
            curveGInfChain.mBrush = Qt::NoBrush;
            curveGInfChain.mIsRectFromZero = false;

            curveGInfChains.append(curveGInfChain);

            GraphCurve curveGSupChain;
            curveGSupChain.mName = QString("G Sup Chain ") + QString::number(i);
            curveGSupChain.mPen = QPen(Painting::chainColors[i], 1, Qt::CustomDashLine);
            curveGSupChain.mPen.setDashPattern(QList<qreal>{5, 5});
            curveGSupChain.mBrush = Qt::NoBrush;
            curveGSupChain.mIsRectFromZero = false;

            curveGSupChains.append(curveGSupChain);

            GraphCurve curveMapChain;
            curveMapChain.mName = QString("Map Chain ") + QString::number(i);
            curveMapChain.mPen = QPen(Painting::chainColors[i], 1, Qt::SolidLine);
            curveMapChain.mBrush = Qt::NoBrush;
            curveMapChain.mIsRectFromZero = false;

            curveMapChain.mType = GraphCurve::eCurveMap;
            curveMapChain.mMap = mComposanteGChains.at(i).mapG;

            if (tmaxFormated > tminFormated) {
                curveMapChain.mMap.setRangeX(tminFormated, tmaxFormated);

            } else {
                curveMapChain.mMap.setRangeX(tmaxFormated, tminFormated);
                // we must reflect the map
                // CurveMap displayMap (0, 0);
                CurveMap displayMap (curveMap.mMap._row, curveMap.mMap._column);

                int c  = curveMap.mMap._column-1;
                unsigned i = 0;
                while ( c >= 0) {
                    for (unsigned r = 0; r < curveMap.mMap._row ; r++) {
                         displayMap.data[i++] = mComposanteG.mapG.at(c, r);
                       // displayMap.data.push_back(mComposanteGChains[i].mapG.at(c, r));
                    }
                    c--;
                }
                curveMapChain.mMap.data = std::move(displayMap.data);
            }

            curveMapChains.append(curveMapChain);
        }


        for (size_t idx = 0; idx < mComposanteG.vecG.size() ; ++idx) {

            t = DateUtils::convertToAppSettingsFormat(idx*step + tmin);

            curveG.mData.insert(t, mComposanteG.vecG[idx]);
            // 95% envelope  https://en.wikipedia.org/wiki/1.96
            curveGSup.mData.insert(t, mComposanteG.vecG[idx] + 1.96 * sqrt(mComposanteG.vecVarG[idx]));
            curveGInf.mData.insert(t, mComposanteG.vecG[idx] - 1.96 * sqrt(mComposanteG.vecVarG[idx]));

            // 68% envelope
            //curveGSup.mData.insert(t, mComposanteG.vecG.at(idx) + 1. * mComposanteG.vecGErr.at(idx));
            //curveGInf.mData.insert(t, mComposanteG.vecG.at(idx) - 1. * mComposanteG.vecGErr.at(idx));

            for (int i = 0; i<curveGChains.size(); ++i) {
                curveGChains[i].mData.insert(t, mComposanteGChains.at(i).vecG[idx]);
                curveGSupChains[i].mData.insert(t, mComposanteGChains.at(i).vecG[idx] + 1.96 * sqrt(mComposanteGChains.at(i).vecVarG[idx]));
                curveGInfChains[i].mData.insert(t, mComposanteGChains.at(i).vecG[idx] - 1.96 * sqrt(mComposanteGChains.at(i).vecVarG[idx]));
            }
        }

        QColor envColor (119, 95, 49);
        envColor.setAlpha(30);
        curveGEnv = generateShapeCurve(curveGInf.mData, curveGSup.mData,
                                         "G Env",
                                         QColor(119, 95, 49), Qt::DashLine, envColor);
        mGraph->addCurve(curveGEnv);
        mGraph->addCurve(curveMap); // to be draw in first

        mGraph->addCurve(curveG);
       // mGraph->addCurve(curveGSup);
       // mGraph->addCurve(curveGInf);


        for (int i = 0; i<curveGChains.size(); ++i) {
            mGraph->addCurve(curveGChains[i]);
            mGraph->addCurve(curveGInfChains[i]);
            mGraph->addCurve(curveGSupChains[i]);
        }

        for (auto&& cMapC: curveMapChains) {
            mGraph->addCurve(cMapC);
        }
        // must be put at the end to print the points above
        mGraph->addCurve(curveEventsPoints);
        mGraph->addCurve(curveDataPoints);
    }

    else if (mCurrentVariableList.contains(eGP)) {
        GraphCurve curveGP;
        curveGP.mName = tr("G Prime");
        curveGP.mPen = QPen(QColor(119, 95, 49),1, Qt::SolidLine);//QColor(154, 80, 225), 1, Qt::SolidLine);
        curveGP.mBrush = Qt::NoBrush;
        curveGP.mIsRectFromZero = false;

       /* GraphCurve curveGPInf;
        curveGPInf.mName = tr("GP Inf");
        curveGPInf.mPen = QPen(QColor(119, 95, 49), 1, Qt::CustomDashLine);
        curveGPInf.mPen.setDashPattern(QList<qreal>{5, 5});
        curveGPInf.mBrush = Qt::NoBrush;
        curveGPInf.mIsHisto = false;
        curveGPInf.mIsRectFromZero = false;

        GraphCurve curveGPSup;
        curveGPSup.mName = tr("GP Sup");
        curveGPSup.mPen = QPen(QColor(119, 95, 49), 1, Qt::CustomDashLine);
        curveGPSup.mPen.setDashPattern(QList<qreal>{5, 5});
        curveGPSup.mBrush = Qt::NoBrush;
        curveGPSup.mIsHisto = false;
        curveGPSup.mIsRectFromZero = false;
        */
        for (size_t idx = 0; idx < mComposanteG.vecG.size() ; ++idx) {
            t = DateUtils::convertToAppSettingsFormat(idx*step + tmin);
            curveGP.mData.insert(t, mComposanteG.vecGP[idx]);
            // 95% envelope  https://en.wikipedia.org/wiki/1.96
 /*
#ifdef DEBUG
            if (mComposanteG.vecVarGP.empty())
                continue;
#endif
            curveGPSup.mData.insert(t, mComposanteG.vecGP[idx] + 1.96 * sqrt(mComposanteG.vecVarGP[idx]));
            curveGPInf.mData.insert(t, mComposanteG.vecGP[idx] - 1.96 * sqrt(mComposanteG.vecVarGP[idx]));
*/
        }

        mGraph->addCurve(curveGP);
        //mGraph->addCurve(curveGPSup);
        //mGraph->addCurve(curveGPInf);

        QList<GraphCurve> curveGPChains;
        for (unsigned i = 0; i < mComposanteGChains.size(); ++i) {
            GraphCurve curveGPChain;
            curveGPChain.mName = QString("G Prime Chain ") + QString::number(i);
            curveGPChain.mPen = QPen(Painting::chainColors[i], 1, Qt::SolidLine);
            curveGPChain.mBrush = Qt::NoBrush;
            curveGPChain.mIsRectFromZero = false;

          /*  GraphCurve curveGPInfChain;
            curveGPInfChain.mName = tr("GP Inf Chain ") + QString::number(i);
            curveGPInfChain.mPen = QPen(Painting::chainColors[i], 1, Qt::CustomDashLine);
            curveGPInfChain.mPen.setDashPattern(QList<qreal>{5, 5});
            curveGPInfChain.mBrush = Qt::NoBrush;
            curveGPInfChain.mIsHisto = false;
            curveGPInfChain.mIsRectFromZero = false;

            GraphCurve curveGPSupChain;
            curveGPSupChain.mName = tr("GP Sup Chain ") + QString::number(i);
            curveGPSupChain.mPen = QPen(Painting::chainColors[i], 1, Qt::CustomDashLine);
            curveGPSupChain.mPen.setDashPattern(QList<qreal>{5, 5});
            curveGPSupChain.mBrush = Qt::NoBrush;
            curveGPSupChain.mIsHisto = false;
            curveGPSupChain.mIsRectFromZero = false;
            */

            for (size_t idx = 0; idx < mComposanteGChains[i].vecGP.size() ; ++idx) {
                t = DateUtils::convertToAppSettingsFormat(idx*step + tmin);
                curveGPChain.mData.insert(t, mComposanteGChains[i].vecGP[idx]);
                /*
#ifdef DEBUG
            if (mComposanteGChains[i].vecVarGP.empty())
                continue;
#endif
                curveGPSupChain.mData.insert(t, mComposanteGChains[i].vecGP[idx] + 1.96 * sqrt(mComposanteGChains[i].vecVarGP[idx]));
                curveGPInfChain.mData.insert(t, mComposanteGChains[i].vecGP[idx] - 1.96 * sqrt(mComposanteGChains[i].vecVarGP[idx]));
                */
            }
            curveGPChains.append(curveGPChain);
           // curveGPChains.append(curveGPSupChain);
           // curveGPChains.append(curveGPInfChain);
        }

        for (auto&& cGP: curveGPChains) {
            mGraph->addCurve(cGP);
        }


    } else if (mCurrentVariableList.contains(eGS)) {
        GraphCurve curveGS;
        curveGS.mName = tr("G Second");
        curveGS.mPen = QPen(QColor(119, 95, 49), 1, Qt::SolidLine); // QColor(236, 105, 64),
        curveGS.mBrush = Qt::NoBrush;
        curveGS.mIsRectFromZero = false;

        for (size_t idx = 0; idx < mComposanteG.vecGS.size() ; ++idx) {
            t = DateUtils::convertToAppSettingsFormat(idx*step + tmin);
            curveGS.mData.insert(t, mComposanteG.vecGS.at(idx));
        }
        mGraph->addCurve(curveGS);

        QList<GraphCurve> curveGSChains;
        for (unsigned i = 0; i < mComposanteGChains.size(); ++i) {
            GraphCurve curveGSChain;
            curveGSChain.mName = QString("G Second Chain ") + QString::number(i);
            curveGSChain.mPen = QPen(Painting::chainColors[i], 1, Qt::SolidLine);
            curveGSChain.mBrush = Qt::NoBrush;
            curveGSChain.mIsRectFromZero = false;
            for (size_t idx = 0; idx < mComposanteGChains[i].vecGS.size() ; ++idx) {
                t = DateUtils::convertToAppSettingsFormat(idx*step + tmin);
                curveGSChain.mData.insert(t, mComposanteGChains[i].vecGS.at(idx));
            }
            curveGSChains.append(curveGSChain);
        }

        for (auto&& cGS: curveGSChains) {
            mGraph->addCurve(cGS);
        }
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
    const bool showMap = showVariableList.contains(eMap);
    const bool showEventsPoints = showVariableList.contains(eGEventsPts);
    const bool showDataPoints = showVariableList.contains(eGDatesPts);
    const bool showGP = showVariableList.contains(eGP);
    const bool showGS = showVariableList.contains(eGS);
    
    mGraph->setCurveVisible("G", mShowAllChains && showG);
    mGraph->setCurveVisible("Map", mShowAllChains && showMap);

    mGraph->setCurveVisible("G Env", mShowAllChains && showGError && showG);
    mGraph->setCurveVisible("G Sup", mShowAllChains && showGError && showG);
    mGraph->setCurveVisible("G Inf", mShowAllChains && showGError && showG);

    mGraph->setCurveVisible("Events Points", showEventsPoints);
    mGraph->setCurveVisible("Data Points", showDataPoints);
    mGraph->setCurveVisible("G Prime", mShowAllChains && showGP);
    mGraph->setCurveVisible("GP Sup", mShowAllChains && showGError && showGP);
    mGraph->setCurveVisible("GP Inf", mShowAllChains && showGError && showGP);
    mGraph->setCurveVisible("G Second", mShowAllChains && showGS);
    
    for (int i = 0; i < mShowChainList.size(); ++i) {
        mGraph->setCurveVisible(QString("G Chain ") + QString::number(i), mShowChainList.at(i) && showG);
        mGraph->setCurveVisible(QString("G Inf Chain ") + QString::number(i), mShowChainList.at(i) && showGError && showG);
        mGraph->setCurveVisible(QString("G Sup Chain ") + QString::number(i), mShowChainList.at(i) && showGError && showG);

        mGraph->setCurveVisible(QString("Map Chain ") + QString::number(i), mShowChainList.at(i) && showMap);

        mGraph->setCurveVisible(QString("G Prime Chain ") + QString::number(i), mShowChainList.at(i) && showGP);
        mGraph->setCurveVisible(QString("GP Inf Chain ") + QString::number(i), mShowChainList.at(i) && showGError && showGP);
        mGraph->setCurveVisible(QString("GP Sup Chain ") + QString::number(i), mShowChainList.at(i) && showGError && showGP);

        mGraph->setCurveVisible(QString("G Second Chain ") + QString::number(i), mShowChainList.at(i) && showGS);
    }
}
