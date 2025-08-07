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

//#include <QtWidgets>


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
    const double step = (mComposanteG.mapG.maxX() - mComposanteG.mapG.minX()) / (mComposanteG.mapG._column -1);
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
        const double tminFormated = DateUtils::convertToAppSettingsFormat(mComposanteG.mapG.minX());
        const double tmaxFormated = DateUtils::convertToAppSettingsFormat(mComposanteG.mapG.maxX());

        if (tmaxFormated > tminFormated) {
            curveMap.mMap.setRangeX(tminFormated, tmaxFormated);

        } else {
            curveMap.mMap.setRangeX(tmaxFormated, tminFormated);
            // we must reflect the map

            CurveMap displayMap (curveMap.mMap._row, curveMap.mMap._column);

            int c  = curveMap.mMap._column-1;

            unsigned i = 0 ;
            while ( c >= 0) {
                for (unsigned r = 0; r < curveMap.mMap._row ; r++) {
                    displayMap.data[i++] = mComposanteG.mapG.at(c, r);

                }
                c--;
            }

            curveMap.mMap.data = std::move(displayMap.data);
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

            if (tmaxFormated > tminFormated) {
                curveMapChain.mMap.setRangeX(tminFormated, tmaxFormated);

            } else {
                curveMapChain.mMap.setRangeX(tmaxFormated, tminFormated);
                // we must reflect the map
                CurveMap displayMap (curveMapChain.mMap._row, curveMapChain.mMap._column);

                int c  = curveMap.mMap._column-1;
                unsigned i = 0;
                while ( c >= 0) {
                    for (unsigned r = 0; r < curveMapChain.mMap._row ; r++) {
                         displayMap.data[i++] = curveMapChain.mMap.at(c, r);
                    }
                    c--;
                }
                curveMapChain.mMap.data = std::move(displayMap.data);
            }

            curveMapChains.append(curveMapChain);
        }


        // Create HPD map
        /*
        GraphCurve hpdMap = curveMap;
        hpdMap.mName = "Hpd_Map";


        double nb_iter = 0;
        for (auto c : mChains) {
            nb_iter += c.mRealyAccepted;
        }

        hpdMap.mMap = densityMap_2_hpdMap(curveMap.mMap, nb_iter);


        {

            hpdMap.mMap.setRangeX(tmaxFormated, tminFormated);
            // we must reflect the map

            CurveMap displayMap (hpdMap.mMap._row, hpdMap.mMap._column);

            int c  = hpdMap.mMap._column-1;

            unsigned i = 0 ;
            while ( c >= 0) {
                for (unsigned r = 0; r < hpdMap.mMap._row ; r++) {
                    displayMap.data[i++] = hpdMap.mMap.at(c, r);

                }
                c--;
            }

            hpdMap.mMap.data = std::move(displayMap.data);
        }
        */

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
            // 95% envelope  https://en.wikipedia.org/wiki/1.96
            curveGSup_Data.insert(t, mComposanteG.vecG[idx] + 1.96 * sqrt(mComposanteG.vecVarG[idx]));
            curveGInf_Data.insert(t, mComposanteG.vecG[idx] - 1.96 * sqrt(mComposanteG.vecVarG[idx]));

            // 68% envelope
            //curveGSup.mData.insert(t, mComposanteG.vecG.at(idx) + 1. * mComposanteG.vecGErr.at(idx));
            //curveGInf.mData.insert(t, mComposanteG.vecG.at(idx) - 1. * mComposanteG.vecGErr.at(idx));

            for (int i = 0; i<mComposanteGChains.size(); ++i) {
                curveG_Data_i[i].insert(t, mComposanteGChains.at(i).vecG[idx]);
                curveGInf_Data_i[i].insert(t, mComposanteGChains.at(i).vecG[idx] - 1.96 * sqrt(mComposanteGChains.at(i).vecVarG[idx]));
                curveGSup_Data_i[i].insert(t, mComposanteGChains.at(i).vecG[idx] + 1.96 * sqrt(mComposanteGChains.at(i).vecVarG[idx]));
            }
        }

        const GraphCurve curveG = FunctionCurve(G_Data, "G", QColor(119, 95, 49) ); // This is the name of the columns when exporting the graphs
        const QColor envColor (119, 95, 49 , 60);

        const GraphCurve &curveGEnv = shapeCurve(curveGInf_Data, curveGSup_Data, "G Env",
                                         QColor(119, 95, 49), Qt::CustomDashLine, envColor);

        mGraph->add_curve(curveMap); // to be draw in first
        //mGraph->add_curve(hpdMap); //

        mGraph->add_curve(curveG); // This is the order of the columns when exporting the graphs
        mGraph->add_curve(curveGEnv);

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
        const double tminFormated = DateUtils::convertToAppSettingsFormat(mComposanteG.mapGP.minX());
        const double tmaxFormated = DateUtils::convertToAppSettingsFormat(mComposanteG.mapGP.maxX());

        if (tmaxFormated > tminFormated) {
            curveMap.mMap.setRangeX(tminFormated, tmaxFormated);

        } else {
            curveMap.mMap.setRangeX(tmaxFormated, tminFormated);
            // we must reflect the map

            CurveMap displayMap (curveMap.mMap._row, curveMap.mMap._column);

            int c  = curveMap.mMap._column-1;

            unsigned i = 0 ;
            while ( c >= 0) {
                for (unsigned r = 0; r < curveMap.mMap._row ; r++) {
                    displayMap.data[i++] = mComposanteG.mapGP.at(c, r);

                }
                c--;
            }

            curveMap.mMap.data = std::move(displayMap.data);
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

            if (tmaxFormated > tminFormated) {
                curveMapChain.mMap.setRangeX(tminFormated, tmaxFormated);

            } else {
                curveMapChain.mMap.setRangeX(tmaxFormated, tminFormated);
                // we must reflect the map
                CurveMap displayMap (curveMapChain.mMap._row, curveMapChain.mMap._column);

                int c  = curveMap.mMap._column-1;
                unsigned i = 0;
                while ( c >= 0) {
                    for (unsigned r = 0; r < curveMapChain.mMap._row ; r++) {
                        displayMap.data[i++] = curveMapChain.mMap.at(c, r);
                    }
                    c--;
                }
                curveMapChain.mMap.data = std::move(displayMap.data);
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
        const GraphCurve &curveGP = FunctionCurve(GP_Data, "G Prime", QColor(119, 95, 49));

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
        const GraphCurve &curveGS = FunctionCurve(GS_Data, "G Second", QColor(119, 95, 49));
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

    mGraph->setCurveVisible("G", mShowAllChains && showG);
    mGraph->setCurveVisible("G Env", mShowAllChains && showGError && showG);

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
    hpdMap.max_value = nb_iter;
    hpdMap.min_value = 0;

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

        while (i < indices.size() && val > 0) { // comme les indices trient les valeurs, lorsqu'on atteint un 0, il n'y a plus de valeur ensuite
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

        // Si le total est différent de nb_iter. Nous n'avons pas toutes les iterations. Donc il faut corriger le résultat
        /*size_t indice_max = indices[0];
        if (sum < nb_iter) {
             for (size_t j = 0; j < indices.size(); ++j) {
                val = cumul_prob[j];
                if (val >0)
                    cumul_prob[j] = nb_iter - 2 * sum + 2* val;

            }

        }*/

        // inversion pour affichage standard, la valeur maximum correspondant au pic
        // pour que le pic ait la plus petite valeur
        // normalisation par nb_iter
        for (unsigned r = 0; r < prob.size(); ++r) {
            if (cumul_prob[r] == 0)
                hpdMap(c, r) = 0; // convention pour l'affichage
            else
                hpdMap(c, r) =  cumul_prob[r];
                //hpdMap(c, r) = static_cast<double>(nb_iter) - cumul_prob[r];
        }
    } // fin boucle c

    return hpdMap;
}
