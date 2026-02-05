/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2026

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
#include "Painting.h"
#include "DateUtils.h"
#include "ModelUtilities.h"
#include "QtUtilities.h"
#include "AppSettings.h"
#include "Painting.h"


GraphViewCurve::GraphViewCurve(QWidget *parent):GraphViewResults(parent)
{
    setMainColor(Painting::borderDark);
    mGraph->setBackgroundColor(QColor(210, 210, 210));
    mGraph->autoAdjustYScale(false);
    mGraph->setYAxisMode(GraphView::eAllTicks);
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

void GraphViewCurve::generateCurves(const graph_t typeGraph, const QList<variable_t> &variableList)
{
    GraphViewResults::generateCurves(typeGraph, variableList);
    mGraph->removeAllCurves();

    mGraph->clearInfos();
    mGraph->resetNothingMessage();
    mGraph->setOverArrow(GraphView::eBothOverflow);
    mGraph->setTipXLab("t");
    mGraph->setYAxisMode(GraphView::eAllTicks);
    mGraph->autoAdjustYScale(false);
    mGraph->mLegendX = DateUtils::getAppSettingsFormatStr();
    mGraph->setFormatFunctX(nullptr);
    mGraph->setBackgroundColor(QColor(230, 230, 230));
    
    const QString &resultsHTML = ModelUtilities::curveResultsHTML(getModel_ptr());
    setNumericalResults(resultsHTML);

    // We use the parameter saved with the map
    const double step = (mComposanteG.mapG.maxX() - mComposanteG.mapG.minX()) / (mComposanteG.mapG.column() -1);
    const double tmin = mComposanteG.mapG.minX();


    // Quantile normal pour 1 - alpha/2
    // 95% envelope  https://en.wikipedia.org/wiki/1.96

    const double threshold = getModel_ptr()->mThreshold;
    const double z_score = zScore(1.0 - threshold * 0.01); // Pour 95% z = 1.96

    if (mCurrentVariableList.contains(eG)) {
        std::vector<CurveRefPts> curveEventsPoints;

        // installation des points de ref

        for (auto& ePts : mEventsPoints) {
            CurveRefPts ref;
            ref.Xmin = DateUtils::convertToAppSettingsFormat(ePts.Xmin);
            ref.Xmax = DateUtils::convertToAppSettingsFormat(ePts.Xmax);
            if (ref.Xmin > ref.Xmax)
                std::swap(ref.Xmin, ref.Xmax);

            ref.Ymin = ePts.Ymin;
            ref.Ymax = ePts.Ymax;
            ref.type = ePts.type;
            ref.color = ePts.color;
            ref.pen = QPen(ePts.color, 1, Qt::SolidLine);
            ref.brush = ePts.color;
            ref.name = "Events Points";
            ref.comment = ePts.comment;
            curveEventsPoints.push_back(ref);
        }

        std::vector<CurveRefPts> curveDataPoints;

        for (auto& dPts : mDataPoints) {
            CurveRefPts ref;
            ref.Xmin = DateUtils::convertToAppSettingsFormat(dPts.Xmin);
            ref.Xmax = DateUtils::convertToAppSettingsFormat(dPts.Xmax);
            if (ref.Xmin > ref.Xmax)
                std::swap(ref.Xmin, ref.Xmax);

            ref.Ymin = dPts.Ymin;
            ref.Ymax = dPts.Ymax;
            ref.type = dPts.type;
            ref.color = dPts.color;
            ref.name = "Data Points";
            ref.comment = dPts.comment;
            curveDataPoints.push_back(ref);

        }
        // fin installation

        GraphCurve curveMap;
        curveMap.mName = "G Map";
        curveMap.mPen = QPen(QColor(107, 174, 214), 1, Qt::SolidLine);//QPen(Qt::black, 1, Qt::SolidLine); //107, 174, 214
        curveMap.mBrush = Qt::NoBrush;
        curveMap.mIsRectFromZero = false;

        curveMap.mType = GraphCurve::eMapData;
        curveMap.mMap = mComposanteG.mapG;
        curveMap.setPalette(AppSettings::mMapPalette);

        const double tminFormated = DateUtils::convertToAppSettingsFormat(mComposanteG.mapG.minX());
        const double tmaxFormated = DateUtils::convertToAppSettingsFormat(mComposanteG.mapG.maxX());

        if (tmaxFormated > tminFormated) {
            curveMap.mMap.setRangeX(tminFormated, tmaxFormated);

        } else {
            curveMap.mMap.setRangeX(tmaxFormated, tminFormated);
            // we must reflect the map

            CurveMap displayMap (curveMap.mMap.row(), curveMap.mMap.column());

            int c  = curveMap.mMap.column() - 1;

            unsigned i = 0 ;
            while ( c >= 0) {
                for (unsigned r = 0; r < curveMap.mMap.row() ; r++) {
                    displayMap[i++] = mComposanteG.mapG.at(c, r);

                }
                c--;
            }

            curveMap.mMap.setData(displayMap.Data());
        }


        QList<GraphCurve> curveMapChains;

        for (unsigned i = 0; i < mComposanteGChains.size(); ++i) {

            GraphCurve curveMapChain;
            curveMapChain.mName = "G Map Chain " + QString::number(i);
            curveMapChain.mPen = QPen(Painting::chainColors[i], 1, Qt::SolidLine);
            curveMapChain.mBrush = Qt::NoBrush;
            curveMapChain.mIsRectFromZero = false;

            curveMapChain.mType = GraphCurve::eMapData;
            curveMapChain.mMap = mComposanteGChains.at(i).mapG;

            const QColor color_0 = QColor(Painting::chainColors[i].red(),
                                   Painting::chainColors[i].green(),
                                   Painting::chainColors[i].blue(),
                                   0);
            const QColor color_1 = QColor(Painting::chainColors[i].red(),
                                   Painting::chainColors[i].green(),
                                   Painting::chainColors[i].blue(),
                                   255);

            std::vector<ColorStop> chainColorStop {
                {0.0, color_0},
                {1.0, color_1}
            };
            curveMapChain.setColorStops(chainColorStop);

            if (tmaxFormated > tminFormated) {
                curveMapChain.mMap.setRangeX(tminFormated, tmaxFormated);

            } else {
                curveMapChain.mMap.setRangeX(tmaxFormated, tminFormated);
                // we must reflect the map
                CurveMap displayMap (curveMapChain.mMap.row(), curveMapChain.mMap.column());

                int c  = curveMap.mMap.column() - 1;
                unsigned i = 0;
                while ( c >= 0) {
                    for (unsigned r = 0; r < curveMapChain.mMap.row() ; r++) {
                         displayMap[i++] = curveMapChain.mMap.at(c, r);
                    }
                    c--;
                }
                curveMapChain.mMap.setData(displayMap.Data());
            }

            curveMapChains.append(curveMapChain);
        }


        // Create HPD Env --------

        std::vector<int> min_indices, max_indices;
        densityMap_2_thresholdIndices_optimized(curveMap.mMap, threshold, min_indices, max_indices);

        QMap<type_data, type_data> curveHPDMid_Data ;
        QMap<type_data, type_data> curveHPDSup_Data ;
        QMap<type_data, type_data> curveHPDInf_Data ;
        type_data Ymin_map = curveMap.mMap.minY();
        type_data Ymax_map = curveMap.mMap.maxY();
        type_data step_map_Y = (Ymax_map - Ymin_map) / (curveMap.mMap.row() -1);
        // les temps de la map sont déjà convertis avant
        type_data tmin_map = curveMap.mMap.minX();
        type_data tmax_map = curveMap.mMap.maxX();
        type_data step_map_t = (tmax_map - tmin_map) / (curveMap.mMap.column() -1);
        for (unsigned c = 0; c < curveMap.mMap.column() ; c++) {

            const double t = c * step_map_t + tmin_map;
            auto val_inf = min_indices[c] * step_map_Y + Ymin_map;
            auto val_sup = max_indices[c] * step_map_Y + Ymin_map;
            curveHPDMid_Data.insert(t, (val_sup + val_inf) / 2.0);
            curveHPDInf_Data.insert(t, val_inf);
            curveHPDSup_Data.insert(t, val_sup);
        }

        //auto hbwd = 0.02 * curveMap.mMap.column();
        auto hbwd = 0.005 * curveMap.mMap.column();
        curveHPDMid_Data = gaussian_filter_simple(curveHPDMid_Data, hbwd);
        curveHPDInf_Data = gaussian_filter_simple(curveHPDInf_Data, hbwd);
        curveHPDSup_Data = gaussian_filter_simple(curveHPDSup_Data, hbwd);

        const QColor envHPDColor (162, 47, 52, 200);

        const GraphCurve curveHPD = FunctionCurve(curveHPDMid_Data, "G HPD Mid", envHPDColor );
        const GraphCurve &curveHPDEnv = shapeCurve(curveHPDInf_Data, curveHPDSup_Data, "G HPD Env",
                                                 envHPDColor, Qt::CustomDashLine, Qt::NoBrush);
        // ---- End HPD Env



        // create Gaussian G curve
        QMap<type_data, type_data> G_Data ;
        QMap<type_data, type_data> curveGSup_Data ;
        QMap<type_data, type_data> curveGInf_Data ;

        std::vector<QMap<type_data, type_data>> curveG_Data_i (mComposanteGChains.size()) ;
        std::vector<QMap<type_data, type_data>> curveGInf_Data_i (mComposanteGChains.size()) ;
        std::vector<QMap<type_data, type_data>> curveGSup_Data_i (mComposanteGChains.size()) ;

        for (size_t idx = 0; idx < mComposanteG.vecG.size() ; ++idx) {

            const double t = DateUtils::convertToAppSettingsFormat(idx*step + tmin);

            G_Data.insert(t, mComposanteG.vecG[idx]);

            curveGSup_Data.insert(t, mComposanteG.vecG[idx] + z_score * sqrt(mComposanteG.vecVarG[idx]));
            curveGInf_Data.insert(t, mComposanteG.vecG[idx] - z_score * sqrt(mComposanteG.vecVarG[idx]));


            for (int i = 0; i<mComposanteGChains.size(); ++i) {
                curveG_Data_i[i].insert(t, mComposanteGChains.at(i).vecG[idx]);
                curveGInf_Data_i[i].insert(t, mComposanteGChains.at(i).vecG[idx] - z_score * sqrt(mComposanteGChains.at(i).vecVarG[idx]));
                curveGSup_Data_i[i].insert(t, mComposanteGChains.at(i).vecG[idx] + z_score * sqrt(mComposanteGChains.at(i).vecVarG[idx]));
            }
        }

        const GraphCurve curveMean = FunctionCurve(G_Data, "G Mean", Painting::mainColorDark ); // This is the name of the columns when exporting the graphs

        const GraphCurve &curveGaussEnv = shapeCurve(curveGInf_Data, curveGSup_Data, "G Gauss Env",
                                         Painting::mainColorDark, Qt::CustomDashLine, Qt::NoBrush);

        mGraph->add_curve(curveMap); // to be draw in first


        mGraph->add_curve(curveMean); // This is the order of the columns when exporting the graphs
        mGraph->add_curve(curveGaussEnv);
        mGraph->add_curve(curveHPD);
        mGraph->add_curve(curveHPDEnv);

        QColor envColor_i;
        for (int i = 0; i<mComposanteGChains.size(); ++i) {

            const GraphCurve &curveG_i = FunctionCurve(curveG_Data_i[i], "G Mean Chain " + QString::number(i),
                                               Painting::chainColors[i]);
            mGraph->add_curve(curveG_i);

            envColor_i  = Painting::chainColors[i];
            envColor_i.setAlpha(30);
            const GraphCurve &curveGEnv_i = shapeCurve(curveGInf_Data_i[i], curveGSup_Data_i[i], "G Gauss Env Chain " + QString::number(i),
                                             Painting::chainColors[i], Qt::CustomDashLine, envColor_i);
            mGraph->add_curve(curveGEnv_i);

        }

        for (auto&& c: curveMapChains) {
            mGraph->add_curve(c);
        }
        // must be put at the end to print the points above
        mGraph->set_points(curveEventsPoints);
        mGraph->insert_points(curveDataPoints);
    }

    else if (mCurrentVariableList.contains(eGP)) {

        GraphCurve curveMap;
        curveMap.mName = "G Prime Map";
        curveMap.mPen = QPen(Qt::black, 1, Qt::SolidLine);
        curveMap.mBrush = Qt::NoBrush;
        curveMap.mIsRectFromZero = false;


        curveMap.mType = GraphCurve::eMapData;
        curveMap.mMap = mComposanteG.mapGP;
        curveMap.setPalette(AppSettings::mMapPalette);// ColorPalette::TemperatureSoftDensity);

        const double tminFormated = DateUtils::convertToAppSettingsFormat(mComposanteG.mapGP.minX());
        const double tmaxFormated = DateUtils::convertToAppSettingsFormat(mComposanteG.mapGP.maxX());

        if (tmaxFormated > tminFormated) {
            curveMap.mMap.setRangeX(tminFormated, tmaxFormated);

        } else {
            curveMap.mMap.setRangeX(tmaxFormated, tminFormated);
            // we must reflect the map

            CurveMap displayMap (curveMap.mMap.row(), curveMap.mMap.column());

            int c  = curveMap.mMap.column() - 1;

            unsigned i = 0 ;
            while ( c >= 0) {
                for (unsigned r = 0; r < curveMap.mMap.row() ; r++) {
                    displayMap[i++] = mComposanteG.mapGP.at(c, r);

                }
                c--;
            }

            curveMap.mMap.setData(displayMap.Data());
        }

        QList<GraphCurve> curveMapChains;

        for (unsigned i = 0; i < mComposanteGChains.size(); ++i) {

            GraphCurve curveMapChain;
            curveMapChain.mName = "G Prime Map Chain " + QString::number(i);
            curveMapChain.mPen = QPen(Painting::chainColors[i], 1, Qt::SolidLine);
            curveMapChain.mBrush = Qt::NoBrush;
            curveMapChain.mIsRectFromZero = false;

            curveMapChain.mType = GraphCurve::eMapData;
            curveMapChain.mMap = mComposanteGChains.at(i).mapGP;

            const QColor color_0 = QColor(Painting::chainColors[i].red(),
                                          Painting::chainColors[i].green(),
                                          Painting::chainColors[i].blue(),
                                          0);
            const QColor color_1 = QColor(Painting::chainColors[i].red(),
                                          Painting::chainColors[i].green(),
                                          Painting::chainColors[i].blue(),
                                          255);

            std::vector<ColorStop> chainColorStop {
                {0.0, color_0},
                {1.0, color_1}
            };
            curveMapChain.setColorStops(chainColorStop);

            if (tmaxFormated > tminFormated) {
                curveMapChain.mMap.setRangeX(tminFormated, tmaxFormated);

            } else {
                curveMapChain.mMap.setRangeX(tmaxFormated, tminFormated);
                // we must reflect the map
                CurveMap displayMap (curveMapChain.mMap.row(), curveMapChain.mMap.column());

                int c  = curveMap.mMap.column() - 1;
                unsigned i = 0;
                while ( c >= 0) {
                    for (unsigned r = 0; r < curveMapChain.mMap.row() ; r++) {
                        displayMap[i++] = curveMapChain.mMap.at(c, r);
                    }
                    c--;
                }
                curveMapChain.mMap.setData(displayMap.Data());
            }

            curveMapChains.append(curveMapChain);
        }

        for (auto&& c: curveMapChains) {
            mGraph->add_curve(c);
        }

        double threshold = getModel_ptr()->mThreshold;
        // Create HPD Env --------

        std::vector<int> min_indices, max_indices;
        densityMap_2_thresholdIndices_optimized(curveMap.mMap, threshold, min_indices, max_indices);

        QMap<type_data, type_data> curveHPDMid_Data ;
        QMap<type_data, type_data> curveHPDSup_Data ;
        QMap<type_data, type_data> curveHPDInf_Data ;
        type_data Ymin_map = curveMap.mMap.minY();
        type_data Ymax_map = curveMap.mMap.maxY();
        type_data step_map_Y = (Ymax_map - Ymin_map) / (curveMap.mMap.row() -1);
        // les temps de la map sont déjà convertis avant
        type_data tmin_map = curveMap.mMap.minX();
        type_data tmax_map = curveMap.mMap.maxX();
        type_data step_map_t = (tmax_map - tmin_map) / (curveMap.mMap.column() -1);
        for (unsigned c = 0; c < curveMap.mMap.column() ; c++) {

            const double t = c * step_map_t + tmin_map;
            auto val_inf = min_indices[c] * step_map_Y + Ymin_map;
            auto val_sup = max_indices[c] * step_map_Y + Ymin_map;
            curveHPDMid_Data.insert(t, (val_sup + val_inf) / 2.0);
            curveHPDInf_Data.insert(t, val_inf);
            curveHPDSup_Data.insert(t, val_sup);
        }

        //auto hbwd = 0.02 * curveMap.mMap.column();
        auto hbwd = 0.005 * curveMap.mMap.column();
        curveHPDMid_Data = gaussian_filter_simple(curveHPDMid_Data, hbwd);
        curveHPDInf_Data = gaussian_filter_simple(curveHPDInf_Data, hbwd);
        curveHPDSup_Data = gaussian_filter_simple(curveHPDSup_Data, hbwd);

        const QColor envHPDColor (162, 47, 52, 200);

        const GraphCurve curveHPD = FunctionCurve(curveHPDMid_Data, "G Prime HPD Mid", envHPDColor );
        const GraphCurve curveHPDEnv = shapeCurve(curveHPDInf_Data, curveHPDSup_Data, "G Prime HPD Env",
                                                   envHPDColor, Qt::CustomDashLine, Qt::NoBrush);


        // ---- End HPD Env



        GraphCurve curveZero = horizontalLine(0., "G Prime Zero", QColor(219, 01, 01));
        mGraph->add_curve(curveZero);

        // Mean Gaussian curve
        QMap<type_data, type_data> GP_Data, GPInf_Data, GPSup_Data ;
        for (size_t idx = 0; idx < mComposanteG.vecGP.size() ; ++idx) {
            double t = DateUtils::convertToAppSettingsFormat(idx*step + tmin);
            GP_Data.insert(t, mComposanteG.vecGP[idx]);
            GPInf_Data.insert(t, mComposanteG.vecGP[idx] - z_score * sqrt(mComposanteG.vecVarGP[idx]));
            GPSup_Data.insert(t, mComposanteG.vecGP[idx] + z_score * sqrt(mComposanteG.vecVarGP[idx]));


        }

        const GraphCurve &curveMean = FunctionCurve(GP_Data, "G Prime Mean", Painting::mainColorDark);

        const GraphCurve &curveGaussEnv = shapeCurve(GPInf_Data, GPSup_Data, "G Prime Gauss Env",
                                                     Painting::mainColorDark, Qt::CustomDashLine, Qt::NoBrush);


        mGraph->add_curve(curveMap); // to be draw in first
        mGraph->add_curve(curveMean);
        mGraph->add_curve(curveGaussEnv);
        mGraph->add_curve(curveHPD);
        mGraph->add_curve(curveHPDEnv);

        std::vector<QMap<type_data, type_data>> GP_Data_i (mComposanteGChains.size()) ;

        QColor envColor_i;
        for (unsigned i = 0; i < mComposanteGChains.size(); ++i) {

            QMap<type_data, type_data> GP_Data_i, GPInf_Data_i,GPSup_Data_i  ;
            for (size_t idx = 0; idx < mComposanteGChains[i].vecGP.size() ; ++idx) {
                double t = DateUtils::convertToAppSettingsFormat(idx*step + tmin); // il faut convertir t
                GP_Data_i.insert(t , mComposanteGChains[i].vecGP[idx]);

                GPInf_Data_i.insert(t, mComposanteGChains.at(i).vecGP[idx] - z_score * sqrt(mComposanteGChains.at(i).vecVarGP[idx]));
                GPSup_Data_i.insert(t, mComposanteGChains.at(i).vecGP[idx] + z_score * sqrt(mComposanteGChains.at(i).vecVarGP[idx]));

            }
            const GraphCurve &curveGPChain = FunctionCurve(GP_Data_i, "G Prime Mean Chain " + QString::number(i), Painting::chainColors[i]);
            mGraph->add_curve(curveGPChain);

            envColor_i  = Painting::chainColors[i];
            envColor_i.setAlpha(30);
            const GraphCurve &curveGEnv_i = shapeCurve(GPInf_Data_i, GPSup_Data_i, "G Prime Gauss Env Chain " + QString::number(i),
                                                       Painting::chainColors[i], Qt::CustomDashLine, envColor_i);
            mGraph->add_curve(curveGEnv_i);

        }
        mGraph->setTipYLab("Rate");
        mGraph->setYAxisMode(GraphView::eAllTicks);
        mGraph->autoAdjustYScale(false);


    } else if (mCurrentVariableList.contains(eGS)) {

        QMap<type_data, type_data> GS_Data;
        for (size_t idx = 0; idx < mComposanteG.vecGS.size() ; ++idx) {
            GS_Data.insert( DateUtils::convertToAppSettingsFormat(idx*step + tmin), mComposanteG.vecGS.at(idx));
        }
        const GraphCurve &curveGS = FunctionCurve(GS_Data, "G Second", Painting::mainColorDark);
        mGraph->add_curve(curveGS);

        for (unsigned i = 0; i < mComposanteGChains.size(); ++i) {

            QMap<type_data, type_data> GS_Data_i;
            for (size_t idx = 0; idx < mComposanteGChains[i].vecGS.size() ; ++idx) {
                GS_Data_i.insert( DateUtils::convertToAppSettingsFormat(idx*step + tmin), mComposanteGChains[i].vecGS.at(idx));
            }
            const GraphCurve &curveGSChain = FunctionCurve(GS_Data_i, "G Second Chain " + QString::number(i), Painting::chainColors[i]);
            mGraph->add_curve(curveGSChain);

        }
        mGraph->setTipYLab("Acc.");

    }

    /* ------------------------------------------------------------
     *   Add zones outside study period
     * ------------------------------------------------------------*/

    const GraphZone zoneMin (-std::numeric_limits<double>::max(), mSettings.getTminFormated());
    mGraph->add_zone(zoneMin);

    const GraphZone zoneMax (mSettings.getTmaxFormated(), std::numeric_limits<double>::max());
    mGraph->add_zone(zoneMax);

    mGraph->setTipXLab(tr("t"));

}

void GraphViewCurve::updateCurvesToShowForG(bool showAllChains, QList<bool> showChainList, const QList<variable_t> &showVariableList, const Scale scale)
{
    // From GraphViewResults::updateCurvesToShow
    mShowAllChains = showAllChains;
    mShowChainList = showChainList;
    mShowVariableList = showVariableList;
    
    const bool showG = showVariableList.contains(eG);
    const bool showGGauss = showVariableList.contains(eGGauss);
    const bool showGHpd = showVariableList.contains(eGHpd);
    const bool showMap = showVariableList.contains(eMap);
    const bool showEventsPoints = showVariableList.contains(eGEventsPts);
    const bool showDataPoints = showVariableList.contains(eGDatesPts);

    const bool showGP = showVariableList.contains(eGP);
    const bool showGPGauss = showVariableList.contains(eGPGauss);
    const bool showGPHpd = showVariableList.contains(eGPHpd);
    const bool showGPMap = showVariableList.contains(eGPMap);

    const bool showGS = showVariableList.contains(eGS);

    if (!mGraph->autoAdjustY()) {
        mGraph->setRangeY(scale.min, scale.max);
        mGraph->setYScaleDivision(scale);
    }

    // set_points_visible ne fait pas repainGraph, il vaut mieux le faire avant setCurveVisible qui fait repaintGraph
    mGraph->set_points_visible("Events Points", showEventsPoints);
    mGraph->set_points_visible("Data Points", showDataPoints);

    QStringList curvesToShow;

    if (mShowAllChains) {
        if (showG) {
            if (showMap) {
                curvesToShow << "G Map";
            }

            if (showGGauss) {
                curvesToShow << "G Mean";
                curvesToShow << "G Gauss Env";
            }
            if (showGHpd) {
                curvesToShow << "G HPD Mid" << "G HPD Env";
            }
        }

        if (showGP) {
            curvesToShow << "G Prime Zero";
            if (showGPGauss) {
                curvesToShow << "G Prime Mean";
                curvesToShow << "G Prime Gauss Env";
            }
            if (showGPMap) {
                curvesToShow << "G Prime Map";

            }
            if (showGPHpd) {
                curvesToShow << "G Prime HPD Mid" << "G Prime HPD Env";
            }
        }


        if (showGS) {
            curvesToShow << "G Second";
        }
    }


    // Ajouter les chaînes individuelles
    for (int i = 0; i < mShowChainList.size(); ++i) {
        if (mShowChainList.at(i)) {

            if (showG) {
                if (showMap) {
                    curvesToShow << QString("G Map Chain %1").arg(i);
                }
                if (showGGauss) {
                    curvesToShow << QString("G Mean Chain %1").arg(i);
                    curvesToShow << QString("G Gauss Env Chain %1").arg(i);
                }
            }
            if (showGP) {
                if (showGPGauss) {
                    curvesToShow << QString("G Prime Mean Chain %1").arg(i);
                    curvesToShow << QString("G Prime Gauss Env Chain %1").arg(i);
                }
                if (showGPMap) {
                    curvesToShow << QString("G Prime Map Chain %1").arg(i);
                }
            }
            if (showGS) {
                curvesToShow << QString("G Second Chain %1").arg(i);
            }
        }
    }

    mGraph->setCurveVisible(curvesToShow, true);




}

CurveMap GraphViewCurve::densityMap_2_hpdMap (const CurveMap& densityMap, int nb_iter)
{
    CurveMap hpdMap (densityMap);
    hpdMap.setMaxValue(nb_iter);
    hpdMap.setMinValue(0);

    const size_t numRows = densityMap.row();
    // transforme la mapCurve en hpd

    for (unsigned c = 1 ; c < densityMap.column(); c++) {

        std::vector<double> prob(numRows);
        for (unsigned r = 0; r < prob.size(); ++r)
            prob[r] = densityMap(c, r);

        //prob = smooth_histogram(prob, 100);
        // Trier les indices par probabilité décroissante
        std::vector<size_t> indices(prob.size());
        std::iota(indices.begin(), indices.end(), 0);

        std::sort(indices.begin(), indices.end(), [&](size_t i, size_t j) {
            return prob[i] > prob[j];
        });

        // Création vecteur cumul_prob avec les mêmes indices que densityMap

        std::vector<double> cumul_prob(prob.size(), 0.0);
        double sum = 0.0;

        size_t i = 0;
        double val = prob[indices[0]]; //to init while loop
        //si la valeur=0, c'est qu'il n'y a pas de courbe, donc par convention pour l'affichage
        // La probabilité reste à 0

        while (i < indices.size() && val > 0) { // comme "indices" trient les valeurs, lorsqu'on atteint un 0, il n'y a plus de valeur ensuite
            size_t j = i + 1;
            val = prob[indices[i]];
            double group_sum = val;

            // On regroupe les valeurs identiques
            if ( val > 0) {
                while (j < indices.size() && prob[indices[j]] == val) {
                    group_sum += prob[indices[j]];
                    ++j;
                }
                // On ajoute la somme à tous les indices de cette valeur
                sum += group_sum;
                for (size_t k = i; k < j; ++k) {
                    cumul_prob[indices[k]] = sum;
                }

                i = j;

            } else {
                break;

            }

        }

        for (unsigned r = 0; r < prob.size(); ++r) {
            hpdMap(c, r) =  cumul_prob[r];

        }
    } // fin boucle c

    return hpdMap;
}

/**
 * @brief Version optimisée avec early exit pour de meilleures performances.
 *
 * Cette version s'arrête dès que le seuil est atteint pour optimiser les performances
 * sur de grandes matrices avec des seuils élevés.
 *
 * @param densityMap La carte de densité d'entrée
 * @param threshold La taux seuil [0 : 100]
 * @param min_indices Vecteur de sortie pour les indices minimum
 * @param max_indices Vecteur de sortie pour les indices maximum
 */
/*
void GraphViewCurve::densityMap_2_thresholdIndices_optimized(const CurveMap& densityMap,
                                             double threshold,
                                             std::vector<int>& min_indices,
                                             std::vector<int>& max_indices)
{
    const size_t numRows = densityMap.row();
    const size_t numCols = densityMap.column();

    min_indices.assign(numCols, -1);
    max_indices.assign(numCols, -1);

    for (unsigned c = 0; c < numCols; ++c) {
        std::vector<double> prob(numRows);
        double total_prob = 0.0;

        // Extraire les probabilités pour cette colonne
        for (unsigned r = 0; r < numRows; ++r) {
            prob[r] = densityMap(c, r);
            total_prob += prob[r];
        }

        if (total_prob <= 0) continue;  // Pas de données

        // Calculer le seuil absolu à atteindre
        const double threshold_absolute = threshold / 100.0 * total_prob;

        // Trier par probabilité décroissante
        std::vector<size_t> indices(numRows);
        std::iota(indices.begin(), indices.end(), 0);
        std::sort(indices.begin(), indices.end(), [&](size_t i, size_t j) {
            return prob[i] > prob[j];
        });

        // Traitement par groupes de valeurs identiques (comme dans l'original)
        std::vector<bool> above_threshold(numRows, false);
        double cumulative = 0.0;
        size_t i = 0;
        double val = prob[indices[0]]; // correspond à la densité max

        // Si la valeur initiale est 0, pas de données dans cette colonne
        if (val <= 0) continue;

        while (i < indices.size() && val > 0) {
            val = prob[indices[i]];

            if (val > 0 && cumulative <= threshold_absolute) {

                double group_sum = val;
                above_threshold[indices[i]] = true;
                size_t j = i + 1;
                // Regrouper TOUTES les valeurs identiques
                while (j < indices.size() && prob[indices[j]] == val) {
                    group_sum += prob[indices[j]];
                    above_threshold[indices[j]] = true;
                    ++j;
                }

                // Mettre à jour la somme cumulative avec tout le groupe
                cumulative += group_sum;

                i = j;

            } else {
                break;
            }
        }

        // Trouver les indices min et max parmi ceux qui dépassent le seuil
        for (unsigned r = 0; r < numRows; ++r) {
            if (above_threshold[r]) {
                if (min_indices[c] == -1 || r < static_cast<unsigned>(min_indices[c])) {
                    min_indices[c] = static_cast<int>(r);
                }
                if (max_indices[c] == -1 || r > static_cast<unsigned>(max_indices[c])) {
                    max_indices[c] = static_cast<int>(r);
                }
            }
        }
        //qDebug() << " colonne = " <<c << " min_indices[c]= " << min_indices[c] << " max_indices= " << max_indices[c];
    }
}
*/

inline double gaussianKernel(double u) noexcept {
    return std::exp(-0.5 * u * u) / std::sqrt(2.0 * M_PI);
}

/**
 * @brief LOOCV rapide pour lissage par noyau gaussien sur std::map<double,double>.
 *
 * Utilise la formule :
 *   ŷ_{-i}(x_i) = (ŷ(x_i) - H_ii * y_i) / (1 - H_ii)
 * avec
 *   ŷ(x_i) = (Σ_j K((x_i-x_j)/h) * y_j) / Σ_j K((x_i-x_j)/h)
 *   H_ii   = K(0) / Σ_j K((x_i-x_j)/h)
 *
 * @param data Données sous forme de std::map<double,double>.
 * @param h Largeur du noyau (bandwidth).
 * @return Score CV(h), plus petit = meilleur.
 */
inline double loocv_fast(const std::map<double,double>& data, double h) noexcept {
    const int n = static_cast<int>(data.size());
    if (n <= 1) return std::numeric_limits<double>::max();

    std::vector<double> yhat(n, 0.0);
    std::vector<double> Hdiag(n, 0.0);

    // Stockage temporaire pour itérateurs
    std::vector<double> xs, ys;
    xs.reserve(n);
    ys.reserve(n);
    for (auto& kv : data) {
        xs.push_back(kv.first);
        ys.push_back(kv.second);
    }

    // 1. Calcul de ŷ(x_i) et H_ii
    for (int i = 0; i < n; ++i) {
        double xi = xs[i];
        double num = 0.0, den = 0.0;
        for (int j = 0; j < n; ++j) {
            double w = gaussianKernel((xi - xs[j]) / h);
            num += w * ys[j];
            den += w;
        }
        yhat[i] = (den > 0.0) ? num / den : 0.0;
        Hdiag[i] = gaussianKernel(0.0) / den;  // poids propre
    }

    // 2. Calcul du CV via la formule optimisée
    double sum = 0.0;
    for (int i = 0; i < n; ++i) {
        double denom = 1.0 - Hdiag[i];
        if (denom <= 1e-14) continue; // éviter division par zéro
        double yhat_minus_i = (yhat[i] - Hdiag[i] * ys[i]) / denom;
        double r = ys[i] - yhat_minus_i;
        sum += r * r;
    }

    return sum / n;
}

/**
 * @brief Recherche du meilleur bandwidth h minimisant loocv_fast(data, h).
 *
 * Méthode :
 *  - balayage grossier en log-échelle (coarse_steps) pour trouver un bon point de départ,
 *  - golden-section search (sur log(h)) pour affiner localement,
 *  - cache des évaluations de loocv_fast pour éviter recomputations.
 *
 * @param data données (std::map<x,y>, x triés et régulièrement espacés)
 * @param h_min borne inférieure (si <=0, choisie automatiquement)
 * @param h_max borne supérieure (si <=0, choisie automatiquement)
 * @param coarse_steps nombre de points pour balayage grossier (ex : 20)
 * @param max_iter nombre max d'itérations pour l'optimisation locale
 * @param tol tolérance sur la différence en log(h) pour arrêter (ex : 1e-6)
 * @return valeur de h qui minimise la LOOCV. Retourne NaN si données insuffisantes.
 *
 * Attention : nécessite que loocv_fast(const std::map<double,double>&, double) soit défini.
 */
// useless
double find_best_bandwidth_map(const std::map<double,double>& data,
                               double h_min = -1.0, double h_max = -1.0,
                               int coarse_steps = 20, int max_iter = 40,
                               double tol = 1e-6)
{
    if (data.size() <= 1) return std::numeric_limits<double>::quiet_NaN();

    // Heuristiques sur les bornes si non fournies
    // calcul dx_min, x_range, stddev(x)
    double x_prev = data.begin()->first;
    double x_first = x_prev;
    double x_last = x_prev;
    double dx_min = std::numeric_limits<double>::max();
    double sumx = x_prev, sumx2 = x_prev*x_prev;
    size_t n = 1;
    for (auto it = std::next(data.begin()); it != data.end(); ++it, ++n) {
        double x = it->first;
        dx_min = std::min(dx_min, std::abs(x - x_prev));
        x_prev = x;
        sumx += x;
        sumx2 += x*x;
        x_last = x;
    }
    const double x_range = x_last - x_first;
    const double meanx = sumx / double(n);
    const double varx = std::max(0.0, sumx2/double(n) - meanx*meanx);
    const double stdx = std::sqrt(varx);

    // if dx_min not set (shouldn't happen), fallback
    if (!std::isfinite(dx_min) || dx_min <= 0.0) dx_min = (x_range > 0.0) ? x_range / (double)n : 1.0;

    // default bounds if not provided
    if (h_min <= 0.0) {
        // ne pas descendre sous la résolution d'échantillonnage
        h_min = std::max(0.5 * dx_min, 1e-12);
    }
    if (h_max <= 0.0) {
        // au plus la moitié de l'intervalle (raisonnable)
        h_max = std::max(x_range * 0.5, std::max(h_min * 10.0, stdx));
    }
    if (!(h_max > h_min)) {
        // invalid bounds
        return std::numeric_limits<double>::quiet_NaN();
    }

    // cache pour éviter recomputation
    std::map<double,double> cache;

    auto eval = [&](double h)->double {
        // clamp h
        if (!(h > 0.0)) return std::numeric_limits<double>::max();
        auto it = cache.find(h);
        if (it != cache.end()) return it->second;
        double v = loocv_fast(data, h); // <- ta fonction optimisée
        cache.emplace(h, v);
        return v;
    };

    // 1) coarse log-grid scan
    double best_h = h_min;
    double best_score = std::numeric_limits<double>::max();
    const double log_hmin = std::log(h_min);
    const double log_hmax = std::log(h_max);
    for (int i = 0; i < coarse_steps; ++i) {
        double t = (coarse_steps == 1) ? 0.5 : double(i) / double(coarse_steps - 1);
        double logh = log_hmin + t * (log_hmax - log_hmin);
        double h = std::exp(logh);
        double s = eval(h);
        if (s < best_score) {
            best_score = s;
            best_h = h;
        }
    }

    // set local search interval around best_h (clamped)
    double left = std::max(h_min, best_h * 0.5);
    double right = std::min(h_max, best_h * 2.0);
    // ensure left < right
    if (!(right > left)) {
        left = h_min;
        right = h_max;
    }

    // 2) golden-section search on log(h)
    const double gr = (std::sqrt(5.0) - 1.0) / 2.0; // ~0.618
    double logL = std::log(left), logR = std::log(right);
    // initialize points
    double logC = logR - gr * (logR - logL);
    double logD = logL + gr * (logR - logL);
    double scoreC = eval(std::exp(logC));
    double scoreD = eval(std::exp(logD));

    for (int iter = 0; iter < max_iter && (logR - logL) > tol; ++iter) {
        if (scoreC < scoreD) {
            logR = logD;
            logD = logC;
            scoreD = scoreC;
            logC = logR - gr * (logR - logL);
            scoreC = eval(std::exp(logC));
        } else {
            logL = logC;
            logC = logD;
            scoreC = scoreD;
            logD = logL + gr * (logR - logL);
            scoreD = eval(std::exp(logD));
        }
    }

    // best among cached evaluated points
    for (const auto &kv : cache) {
        if (kv.second < best_score) {
            best_score = kv.second;
            best_h = kv.first;
        }
    }

    return best_h;
}
