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

#include "GraphViewCurve.h"

#include "GraphView.h"
#include "Painting.h"
#include "DateUtils.h"
#include "ModelUtilities.h"
#include "QtUtilities.h"
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
    mGraph->remove_all_zones();
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
        curveMap.mName = "Map";
        curveMap.mPen = QPen(QColor(107, 174, 214), 1, Qt::SolidLine);//QPen(Qt::black, 1, Qt::SolidLine); //107, 174, 214
        curveMap.mBrush = Qt::NoBrush;
        curveMap.mIsRectFromZero = false;

        curveMap.mType = GraphCurve::eMapData;
        curveMap.mMap = mComposanteG.mapG;
        curveMap.setPalette(ColorPalette::TemperatureSoftDensity);

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
            curveMapChain.mName = "Map Chain " + QString::number(i);
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


        // Create HPD Env
        double threshold = getModel_ptr()->mThreshold;
        std::vector<int> min_indices, max_indices;
        densityMap_2_thresholdIndices_optimized(curveMap.mMap, threshold, min_indices, max_indices);

        QMap<type_data, type_data> curveHPDMid_Data ;
        QMap<type_data, type_data> curveHPDSup_Data ;
        QMap<type_data, type_data> curveHPDInf_Data ;
        type_data Ymin_map = curveMap.mMap.minY();
        type_data Ymax_map = curveMap.mMap.maxY();
        type_data step_map_Y = (Ymax_map - Ymin_map) / (curveMap.mMap.row() -1);

        type_data tmin_map = curveMap.mMap.minX();
        type_data tmax_map = curveMap.mMap.maxX();
        type_data step_map_t = (tmax_map - tmin_map) / (curveMap.mMap.column() -1);
        for (unsigned c = 0; c < curveMap.mMap.column() ; c++) {
            const double t = DateUtils::convertToAppSettingsFormat(c * step_map_t + tmin_map);
            auto val_inf = min_indices[c] * step_map_Y + Ymin_map;
            auto val_sup = max_indices[c] * step_map_Y + Ymin_map;
            curveHPDMid_Data.insert(t, (val_sup + val_inf) / 2.0);
            curveHPDInf_Data.insert(t, val_inf);
            curveHPDSup_Data.insert(t, val_sup);
        }

        auto hbwd = 0.02 * curveMap.mMap.column();
        curveHPDMid_Data = gaussian_filter_simple(curveHPDMid_Data, hbwd);
        curveHPDInf_Data = gaussian_filter_simple(curveHPDInf_Data, hbwd);
        curveHPDSup_Data = gaussian_filter_simple(curveHPDSup_Data, hbwd);

        /*const QColor envHPDColor (Painting::mainGreen.red(),
                              Painting::mainGreen.green(),
                              Painting::mainGreen.blue(),
                              90);*/

        const GraphCurve curveHPD = FunctionCurve(curveHPDMid_Data, "HPD Mid", Painting::mainGreen );
        const GraphCurve &curveHPDEnv = shapeCurve(curveHPDInf_Data, curveHPDSup_Data, "HPD Env",
                                                 Painting::mainGreen, Qt::CustomDashLine, Qt::NoBrush);

        // Quantile normal pour 1 - alpha/2
        // 95% envelope  https://en.wikipedia.org/wiki/1.96
        const double z_score = zScore(1.0 - threshold * 0.01); // Pour 95% z = 1.96


        // create G curve
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

        const GraphCurve curveG = FunctionCurve(G_Data, "G", Painting::mainColorDark ); // This is the name of the columns when exporting the graphs

       /* const QColor envColor (Painting::mainColorDark.red(),
                               Painting::mainColorDark.green(),
                               Painting::mainColorDark.blue(),
                               60);*/

        const GraphCurve &curveGEnv = shapeCurve(curveGInf_Data, curveGSup_Data, "G Env",
                                         Painting::mainColorDark, Qt::CustomDashLine, Qt::NoBrush);

        mGraph->add_curve(curveMap); // to be draw in first
        //mGraph->add_curve(hpdMap); //

        mGraph->add_curve(curveG); // This is the order of the columns when exporting the graphs
        mGraph->add_curve(curveGEnv);
        mGraph->add_curve(curveHPD);
        mGraph->add_curve(curveHPDEnv);

        QColor envColor_i;
        for (int i = 0; i<mComposanteGChains.size(); ++i) {

            const GraphCurve &curveG_i = FunctionCurve(curveG_Data_i[i], "G Chain " + QString::number(i),
                                               Painting::chainColors[i]);
            mGraph->add_curve(curveG_i);

            envColor_i  = Painting::chainColors[i];
            envColor_i.setAlpha(30);
            const GraphCurve &curveGEnv_i = shapeCurve(curveGInf_Data_i[i], curveGSup_Data_i[i], "G Env Chain " + QString::number(i),
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
        curveMap.mName = "Map";
        curveMap.mPen = QPen(Qt::black, 1, Qt::SolidLine);
        curveMap.mBrush = Qt::NoBrush;
        curveMap.mIsRectFromZero = false;


        curveMap.mType = GraphCurve::eMapData;
        curveMap.mMap = mComposanteG.mapGP;
        curveMap.setPalette(ColorPalette::TemperatureSoftDensity);

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
            curveMapChain.mName = "Map Chain " + QString::number(i);
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

        GraphCurve curveGPZero = horizontalLine(0., "G Prime Zero", QColor(219, 01, 01));
        mGraph->add_curve(curveGPZero);

        QMap<type_data, type_data> GP_Data ;
        for (size_t idx = 0; idx < mComposanteG.vecGP.size() ; ++idx) {
            GP_Data.insert(DateUtils::convertToAppSettingsFormat(idx*step + tmin), mComposanteG.vecGP[idx]);
        }
        const GraphCurve &curveGP = FunctionCurve(GP_Data, "G Prime", Painting::mainColorDark);//QColor(119, 95, 49));

        mGraph->add_curve(curveMap); // to be draw in first
        mGraph->add_curve(curveGP);

        std::vector<QMap<type_data, type_data>> GP_Data_i (mComposanteGChains.size()) ;
        for (unsigned i = 0; i < mComposanteGChains.size(); ++i) {

            QMap<type_data, type_data> GP_Data_i ;
            for (size_t idx = 0; idx < mComposanteGChains[i].vecGP.size() ; ++idx) {
                GP_Data_i.insert( DateUtils::convertToAppSettingsFormat(idx*step + tmin), mComposanteGChains[i].vecGP[idx]);
            }
            const GraphCurve &curveGPChain = FunctionCurve(GP_Data_i, "G Prime Chain " + QString::number(i), Painting::chainColors[i]);
            mGraph->add_curve(curveGPChain);

        }
        mGraph->setTipYLab("Rate");
        mGraph->setYAxisMode(GraphView::eAllTicks);
        mGraph->autoAdjustYScale(false);


    } else if (mCurrentVariableList.contains(eGS)) {

        QMap<type_data, type_data> GS_Data;
        for (size_t idx = 0; idx < mComposanteG.vecGS.size() ; ++idx) {
            GS_Data.insert( DateUtils::convertToAppSettingsFormat(idx*step + tmin), mComposanteG.vecGS.at(idx));
        }
        const GraphCurve &curveGS = FunctionCurve(GS_Data, "G Second", Painting::mainColorDark);//QColor(119, 95, 49));
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

    const GraphZone zoneMin (-INFINITY, mSettings.getTminFormated());
    mGraph->add_zone(zoneMin);

    const GraphZone zoneMax (mSettings.getTmaxFormated(), INFINITY);
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
    const bool showGError = showVariableList.contains(eGError);
    const bool showGHpd = showVariableList.contains(eGHpd);
    const bool showMap = showVariableList.contains(eMap);
    const bool showEventsPoints = showVariableList.contains(eGEventsPts);
    const bool showDataPoints = showVariableList.contains(eGDatesPts);
    const bool showGP = showVariableList.contains(eGP);
    const bool showGS = showVariableList.contains(eGS);

    if (!mGraph->autoAdjustY()) {
        mGraph->setRangeY(scale.min, scale.max);
        mGraph->setYScaleDivision(scale);
    }

    mGraph->setCurveVisible("Map", mShowAllChains && (showG||showGP) && showMap);
   // mGraph->setCurveVisible("Hpd_Map", mShowAllChains && (showG||showGP) && showMap);

    mGraph->setCurveVisible("G", mShowAllChains && showGError);
    mGraph->setCurveVisible("G Env", mShowAllChains && showGError && showG);

    mGraph->setCurveVisible("HPD Mid", mShowAllChains && showGHpd);
    mGraph->setCurveVisible("HPD Env", mShowAllChains && showGHpd);

    mGraph->set_points_visible("Events Points", showEventsPoints);
    mGraph->set_points_visible("Data Points", showDataPoints);
    mGraph->setCurveVisible("G Prime", mShowAllChains && showGP);
    mGraph->setCurveVisible("G Prime Zero", showGP);

    mGraph->setCurveVisible("G Second", mShowAllChains && showGS);
    
    for (int i = 0; i < mShowChainList.size(); ++i) {

        mGraph->setCurveVisible(QString("Map Chain ") + QString::number(i), mShowChainList.at(i) && showMap);

        mGraph->setCurveVisible(QString("G Chain ") + QString::number(i), mShowChainList.at(i) && showG);
        mGraph->setCurveVisible(QString("G Env Chain ") + QString::number(i), mShowChainList.at(i) && showGError && showG);

        mGraph->setCurveVisible(QString("G Prime Chain ") + QString::number(i), mShowChainList.at(i) && showGP);

        mGraph->setCurveVisible(QString("G Second Chain ") + QString::number(i), mShowChainList.at(i) && showGS);
    }

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
