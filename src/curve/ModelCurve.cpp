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

#include "ModelCurve.h"
#include "ModelUtilities.h"
//#include "Project.h"
#include "QtUtilities.h"
#include <MainWindow.h>

#include <QFile>
#include <QFileDialog>
#include <qdatastream.h>
#include <qapplication.h>


#include <math.h>


ModelCurve::ModelCurve():Model()
{
    mLambdaSpline.mSupport = MetropolisVariable::eR;
    mLambdaSpline.mFormat = DateUtils::eNumeric;
    mLambdaSpline.mSamplerProposal = MHVariable::eMHAdaptGauss;

    mS02Vg.mSupport = MetropolisVariable::eR;
    mS02Vg.mFormat = DateUtils::eNumeric;
    mS02Vg.mSamplerProposal = MHVariable::eMHAdaptGauss;
}

ModelCurve::~ModelCurve()
{

}

QJsonObject ModelCurve::toJson() const
{
    QJsonObject json = Model::toJson();
    
    json[STATE_CURVE] = mCurveSettings.toJson();
    
    return json;
}

void ModelCurve::fromJson(const QJsonObject& json)
{
    Model::fromJson(json);
    
    if (json.contains(STATE_CURVE)) {
        const QJsonObject settings = json.value(STATE_CURVE).toObject();
        mCurveSettings = CurveSettings::fromJson(settings);
    }

    for (Event*& event: mEvents) {
        if (event->type() ==  Event::eBound ||
            mCurveSettings.mTimeType == CurveSettings::eModeFixed)
               event->mTheta.mSamplerProposal = MHVariable::eFixe;

        else if (event->type() ==  Event::eDefault)
                event->mTheta.mSamplerProposal = MHVariable::eMHAdaptGauss;

        if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed)
            event->mVG.mSamplerProposal = MHVariable::eFixe;
        else
            event->mVG.mSamplerProposal = MHVariable::eMHAdaptGauss;

    }

    if (mCurveSettings.mLambdaSplineType == CurveSettings::eModeFixed)
        mLambdaSpline.mSamplerProposal = MHVariable::eFixe;
    else
        mLambdaSpline.mSamplerProposal = MHVariable::eMHAdaptGauss;

    mS02Vg.mSamplerProposal = MHVariable::eMHAdaptGauss;

}

// Date files read / write
/** @Brief Save .res file, the result of computation and compress it
*
* */
void ModelCurve::saveToFile(QDataStream *out)
{
    Model::saveToFile(out);


    /* -----------------------------------------------------
    *   Write curve data
    * ----------------------------------------------------- */
    // -----------------------------------------------------
    //  Write events VG
    // -----------------------------------------------------

    for (Event*& event : mEvents)
        *out << event->mVG;

    *out << mLambdaSpline;
    *out << mS02Vg;

    *out << (quint32) mSplinesTrace.size();
    for (auto& splin : mSplinesTrace)
        *out << splin;

    *out << mPosteriorMeanG;

    *out << (quint32) mPosteriorMeanGByChain.size();
    for (auto& pMByChain : mPosteriorMeanGByChain)
        *out << pMByChain;

}
/** @Brief Read the .res file, it's the result of the saved computation
*
* */
void ModelCurve::restoreFromFile(QDataStream* in)
{

    Model::restoreFromFile(in);

    generateCorrelations(mChains);

    /* -----------------------------------------------------
    *  Read events VG
    *----------------------------------------------------- */

    for (auto&& e : mEvents)
        *in >> e->mVG;

    /* -----------------------------------------------------
    *   Read curve data
    * ----------------------------------------------------- */
    quint32 tmp32;
    *in >> mLambdaSpline;

    *in >> mS02Vg;

    *in >> tmp32;
    mSplinesTrace.resize(tmp32);
    for (auto& splin : mSplinesTrace)
        *in >> splin;

    *in >> mPosteriorMeanG;

    *in >> tmp32;
    mPosteriorMeanGByChain.resize(tmp32);
    for (auto& pMByChain : mPosteriorMeanGByChain)
        *in >> pMByChain;

    generateCorrelations(mChains);
}

/* C' était le même algorithme que MCMCCurve::memo_PosteriorG()
 */
/** @TODO
 *  memo_Posterior à changer, elle inclue la conversion IDF
 */
PosteriorMeanGComposante ModelCurve::buildCurveAndMap(const int nbPtsX, const int nbPtsY, const char charComp, const bool doMap, const double mapYMin, double mapYMax)
{
    double ymin;
    double ymax;

    PosteriorMeanGComposante compoG;

    if (doMap) {
        if (mapYMin == mapYMax) {
            switch (charComp) {
            case 'X':
                ymin = mPosteriorMeanG.gx.mapG.minY();
                ymax = mPosteriorMeanG.gx.mapG.maxY();
                break;
            case 'Y':
                ymin = mPosteriorMeanG.gy.mapG.minY();
                ymax = mPosteriorMeanG.gy.mapG.maxY();
                break;
            case 'Z':
                ymin = mPosteriorMeanG.gz.mapG.minY();
                ymax = mPosteriorMeanG.gz.mapG.maxY();
                break;
            default:
                ymin = 0.;
                ymax = 100.;
                break;
            }

        } else {
            ymin = mapYMin;
            ymax = mapYMax;
        }

        compoG.mapG = CurveMap (nbPtsX, nbPtsY);
        compoG.mapG.setRangeX(mSettings.mTmin, mSettings.mTmax);
        compoG.mapG.setRangeY(ymin, ymax);
        compoG.mapG.min_value = +INFINITY;
        compoG.mapG.max_value = 0;

    } else {
        ymin = 0;
        ymax = 0;
        compoG.mapG = CurveMap (0, 0);
        compoG.mapG.setRangeX(0, 0);
        compoG.mapG.min_value = 0;
        compoG.mapG.max_value = 0;

    }

    compoG.vecG = std::vector<double> (nbPtsX);
    compoG.vecGP = std::vector<double> (nbPtsX);
    compoG.vecGS = std::vector<double> (nbPtsX);
    compoG.vecVarG = std::vector<double> (nbPtsX);
    compoG.vecVarianceG = std::vector<double> (nbPtsX);
    compoG.vecVarErrG = std::vector<double> (nbPtsX);

    CurveMap& curveMap = compoG.mapG;

    const double stepT = (mSettings.mTmax - mSettings.mTmin) / (nbPtsX - 1);
    const double stepY = (ymax - ymin) / (nbPtsY - 1);

    // 2 - Variables temporaires
    // référence sur variables globales
    std::vector<double>& vecVarG = compoG.vecVarG;
    // Variables temporaires
    // erreur inter spline
    std::vector<double>& vecVarianceG = compoG.vecVarianceG;
    // erreur intra spline
    std::vector<double>& vecVarErrG = compoG.vecVarErrG;


    double t, g, gp, gs, varG, stdG;
    g = 0.;
    gp = 0;
    varG = 0;
    gs = 0;

    double  prevMeanG;

    const double k = 3.; // Le nombre de fois sigma G, pour le calcul de la densité sudr la map
    //double a, b, surfG;

    //int  idxYErrMin, idxYErrMax;

    // Refaire un vecteur pour les composantes X, Y ou Z utile

    auto runTrace = fullRunSplineTrace(mChains);
    std::vector<MCMCSplineComposante> traceCompoXYZ (runTrace.size());
    switch (charComp) {
    case 'X':
         std::transform(runTrace.begin(), runTrace.end(), traceCompoXYZ.begin(), [](MCMCSpline &s){return s.splineX;});
        break;
    case 'Y':
         std::transform(runTrace.begin(), runTrace.end(), traceCompoXYZ.begin(), [](MCMCSpline &s){return s.splineY;});
        break;
    case 'Z':
         std::transform(runTrace.begin(), runTrace.end(), traceCompoXYZ.begin(), [](MCMCSpline &s){return s.splineZ;});
        break;
    default:
        break;
    }

    //Pointeur sur tableau
    std::vector<double>::iterator itVecG, itVecGP, itVecGS;

    // Variables temporaires
    // erreur inter spline
    std::vector<double>::iterator itVecVarianceG;
    // erreur intra spline
    std::vector<double>::iterator itVecVarErrG;


    unsigned n = 0;
    for (auto& splineXYZ : traceCompoXYZ) {
        n++;
        itVecG = compoG.vecG.begin();
        itVecGP = compoG.vecGP.begin();
        itVecGS = compoG.vecGS.begin();
        itVecVarianceG = compoG.vecVarianceG.begin();
        itVecVarErrG = compoG.vecVarErrG.begin();

        // 3 - Calcul pour la composante
        unsigned i0 = 0; // tIdx étant croissant, i0 permet de faire la recherche à l'indice du temps précedent
        for (int idxT = 0; idxT < nbPtsX ; ++idxT) {
            t = (double)idxT * stepT + mSettings.mTmin ;
            valeurs_G_VarG_GP_GS(t, splineXYZ, g, varG, gp, gs, i0);

            // -- Calcul Mean
            prevMeanG = *itVecG;
            *itVecG +=  (g - prevMeanG)/n;

            *itVecGP +=  (gp - *itVecGP)/n;
            *itVecGS +=  (gs - *itVecGS)/n;
            // erreur inter spline
            *itVecVarianceG +=  (g - prevMeanG)*(g - *itVecG);
            // erreur intra spline
            *itVecVarErrG += (varG - *itVecVarErrG) / n  ;

            ++itVecG;
            ++itVecGP;
            ++itVecGS;
            ++itVecVarianceG;
            ++itVecVarErrG;


            // -- Calcul map
            if (doMap) {
                stdG = sqrt(varG);

                // Ajout densité erreur sur Y
                /* il faut utiliser un pas de grille et le coefficient dans la grille dans l'intervalle [a,b] pour N(mu, sigma) est égale à la différence 1/2*(erf((b-mu)/(sigma*sqrt(2)) - erf((a-mu)/(sigma*sqrt(2))
                 * https://en.wikipedia.org/wiki/Error_function
                 */
                const int idxYErrMin = inRange( 0, int((g - k*stdG - ymin) / stepY), nbPtsY-1);
                const int idxYErrMax = inRange( 0, int((g + k*stdG - ymin) / stepY), nbPtsY-1);

                if (idxYErrMin == idxYErrMax && idxYErrMin > 0 && idxYErrMax < nbPtsY-1) {
#ifdef DEBUG
                    if ((curveMap.row()*idxT + idxYErrMin) < (curveMap.row()*curveMap.column()))
                        curveMap(idxT, idxYErrMin) = curveMap.at(idxYErrMin, idxYErrMin) + 1; // correction à faire dans finalize() + 1./nbIter;
                    else
                        qDebug()<<"pb in MCMCLoopCurve::memo_PosteriorG";
#else
                    curveMap(idxT, idxYErrMin) = curveMap.at(idxT, idxYErrMin) + 1.; // correction à faire dans finalize/nbIter ;
#endif

                    curveMap.max_value = std::max(curveMap.max_value, curveMap.at(idxT, idxYErrMin));
                    if (curveMap.at(idxT, idxYErrMin)>0 )
                        curveMap.min_value = std::min(curveMap.max_value, curveMap.at(idxT, idxYErrMin));


                } else if (idxYErrMin != idxYErrMax && 0 <= idxYErrMin && idxYErrMax < nbPtsY) {
                    double* ptr_Ymin = curveMap.ptr_at(idxT, idxYErrMin);
                    double* ptr_Ymax = curveMap.ptr_at(idxT, idxYErrMax);

                    int idErr = idxYErrMin;
                    for (double* ptr_idErr = ptr_Ymin; ptr_idErr <= ptr_Ymax; ptr_idErr++) {
                        const double a = (idErr - 0.5)*stepY + ymin;
                        const double b = (idErr + 0.5)*stepY + ymin;
                        const double surfG = diff_erf(a, b, g, stdG );// correction à faire dans finalyze /nbIter;
                        *ptr_idErr = (*ptr_idErr) + surfG;

                        curveMap.max_value = std::max(curveMap.max_value, *ptr_idErr);

                        if (*ptr_idErr>0 )
                            curveMap.min_value = std::min(curveMap.max_value, *ptr_idErr);

                        idErr++;
                    }
                }
            }

        }
        int tIdx = 0;
        for (auto& vVarG : vecVarG) {
            vVarG = vecVarianceG.at(tIdx)/ n + vecVarErrG.at(tIdx);
            ++tIdx;
        }

    }
    return compoG;
}


void ModelCurve::saveMapToFile(QFile *file, const QString csvSep, const CurveMap& map)
{
    QTextStream output(file);
    const QString version = qApp->applicationName() + " " + qApp->applicationVersion();
    output<<"# " +version+"\r";

    output<<"# Date Format : "+ DateUtils::getAppSettingsFormatStr() +"\r";

    auto mapG = map.data;

    const double stepX = (map.maxX() - map.minX()) / (map._column - 1);
    const double stepY = (map.maxY() - map.minY()) / (map._row - 1);

    /*  Export with date format, the data are stored with the BC/AD format,
     * So you have to reverse the export when the format is in Age
     */
    bool isDateFormat = DateUtils::convertToAppSettingsFormat(map.minX()) < DateUtils::convertToAppSettingsFormat(map.maxX());

    // Header
    if (isDateFormat) {
        output << "Date / Y"<< csvSep;
        for (unsigned c = 0; c < map._column; ++c)  {
            output << stringForCSV(map.minY() + c * stepY) << csvSep;
        }
        output << "\r";

        unsigned i = 0;
        for (unsigned c = 0; c < map._column; ++c)  {

            output << stringForCSV(DateUtils::convertToAppSettingsFormat(map.minX() + c * stepX))  << csvSep;
            for (unsigned r = 0; r < map._row; ++r)  {
                output << stringForCSV(mapG[i++]) << csvSep;
            }
            output << "\r";
        }
    } else {
        output << "Age / Y"<< csvSep;
        for (unsigned c = 0; c < map._column; ++c)  {
            output << stringForCSV(map.minY() + c * stepY) << csvSep;
        }
        output << "\r";

        //unsigned i = 0;
        for (int c = map._column-1 ; c > -1; --c)  {
            output << stringForCSV(DateUtils::convertToAppSettingsFormat(map.minX() + c * stepX))  << csvSep;

            for (unsigned r = 0; r < map._row; ++r)  {
                auto m = std::remove_const<const CurveMap>::type (map).at(c, r);
                output << stringForCSV(m) << csvSep;
            }
            output << "\r";
        }
    }

}

void  ModelCurve::generateResultsLog()
{
    Model::generateResultsLog();

    QString log;
    log += ModelUtilities::curveResultsHTML(this);
    log += "<hr>";

    mLogResults += log;
}

void ModelCurve::generatePosteriorDensities(const QList<ChainSpecs> &chains, int fftLen, double bandwidth)
{
    Model::generatePosteriorDensities(chains, fftLen, bandwidth);

    for (Event* &event : mEvents) {
        event->mVG.updateFormatedTrace();
        event->mVG.generateHistos(chains, fftLen, bandwidth);
    }

    mLambdaSpline.updateFormatedTrace();
    mLambdaSpline.generateHistos(chains, fftLen, bandwidth);

    mS02Vg.updateFormatedTrace();
    mS02Vg.generateHistos(chains, fftLen, bandwidth);
}

void ModelCurve::generateCorrelations(const QList<ChainSpecs> &chains)
{
    Model::generateCorrelations(chains);
    for (auto&& event : mEvents )
        event->mVG.generateCorrelations(chains);

    mLambdaSpline.generateCorrelations(chains);
    mS02Vg.generateCorrelations(chains);
}

void ModelCurve::generateNumericalResults(const QList<ChainSpecs> &chains)
{
    Model::generateNumericalResults(chains);
    for (Event*& event : mEvents)
        event->mVG.generateNumericalResults(chains);

    mLambdaSpline.generateNumericalResults(chains);
    mS02Vg.generateNumericalResults(chains);
}

void ModelCurve::clearThreshold()
{
    Model::clearThreshold();
   // mThreshold = -1.;
    for (Event*& event : mEvents)
        event->mVG.mThresholdUsed = -1.;

    mLambdaSpline.mThresholdUsed = -1.;
    mS02Vg.mThresholdUsed = -1.;
}

void ModelCurve::generateCredibility(const double& thresh)
{
    Model::generateCredibility(thresh);
    for (Event*& event : mEvents) {
        event->mVG.generateCredibility(mChains, thresh);
    }
    mLambdaSpline.generateCredibility(mChains, thresh);
    mS02Vg.generateCredibility(mChains, thresh);
}

void ModelCurve::generateHPD(const double thresh)
{
    Model::generateHPD(thresh);
    for (Event*& event : mEvents) {
        if (event->type() != Event::eBound) {
            event->mVG.generateHPD(thresh);
        }
    }
    
    mLambdaSpline.generateHPD(thresh);
    mS02Vg.generateHPD(thresh);
}

void ModelCurve::clearPosteriorDensities()
{
    Model::clearPosteriorDensities();
    
    for (Event*& event : mEvents) {
        if (event->type() != Event::eBound) {
            event->mVG.mHisto.clear();
            event->mVG.mChainsHistos.clear();
        }
    }
    
    mLambdaSpline.mHisto.clear();
    mLambdaSpline.mChainsHistos.clear();

    mS02Vg.mHisto.clear();
    mS02Vg.mChainsHistos.clear();
}

void ModelCurve::clearCredibilityAndHPD()
{
    Model::clearCredibilityAndHPD();
    
    for (Event*& event : mEvents) {
        if (event->type() != Event::eBound) {
            event->mVG.mHPD.clear();
            event->mVG.mCredibility = QPair<double, double>();
        }
    }
    
    mLambdaSpline.mHPD.clear();
    mLambdaSpline.mCredibility = QPair<double, double>();

    mS02Vg.mHPD.clear();
    mS02Vg.mCredibility = QPair<double, double>();
}

void ModelCurve::clearTraces()
{
    Model::clearTraces();
  /*  for (Event*& event : mEvents)
        event->mVG.reset();
*/
    mLambdaSpline.reset();
    mS02Vg.reset();
}


void ModelCurve::setThresholdToAllModel(const double threshold)
{
    Model::setThresholdToAllModel(threshold);
    
    for (Event*& event : mEvents)
        event->mVG.mThresholdUsed = mThreshold;

    mLambdaSpline.mThresholdUsed = mThreshold;
    mS02Vg.mThresholdUsed = mThreshold;
}



QList<PosteriorMeanGComposante> ModelCurve::getChainsMeanGComposanteX()
{
    QList<PosteriorMeanGComposante> composantes;
    
    for (auto& pByChain : mPosteriorMeanGByChain)
        composantes.append(pByChain.gx);

    return composantes;
}

QList<PosteriorMeanGComposante> ModelCurve::getChainsMeanGComposanteY()
{
    QList<PosteriorMeanGComposante> composantes;

    for (auto& pByChain : mPosteriorMeanGByChain)
        composantes.append(pByChain.gy);

    return composantes;
}

QList<PosteriorMeanGComposante> ModelCurve::getChainsMeanGComposanteZ()
{
    QList<PosteriorMeanGComposante> composantes;
    
    for (auto& pByChain : mPosteriorMeanGByChain)
        composantes.append(pByChain.gz);

    return composantes;
}

void ModelCurve::valeurs_G_VarG_GP_GS(const double t, const MCMCSplineComposante &spline, double& G, double& varG, double& GP, double& GS, unsigned& i0)
{
    unsigned n = spline.vecThetaEvents.size();
    const double tReduce =  reduceTime(t);
    const double t1 = reduceTime(spline.vecThetaEvents.at(0));
    const double tn = reduceTime(spline.vecThetaEvents.at(n-1));
    GP = 0.;
    GS = 0.;
    double h;

     // The first derivative is always constant outside the interval [t1,tn].
     if (tReduce < t1) {
         const double t2 = reduceTime(spline.vecThetaEvents.at(1));

         // ValeurGPrime
         GP = (spline.vecG.at(1) - spline.vecG.at(0)) / (t2 - t1);
         GP -= (t2 - t1) * spline.vecGamma.at(1) / 6.;

         // ValeurG
         G = spline.vecG.at(0) - (t1 - tReduce) * GP;

         // valeurErrG
         varG = spline.vecVarG.at(0);

         // valeurGSeconde
         //GS = 0.;

     } else if (tReduce >= tn) {

         const double tn1 = reduceTime(spline.vecThetaEvents.at(n-2));

         // valeurErrG
         varG = spline.vecVarG.at(n-1);

         // ValeurGPrime
         GP = (spline.vecG.at(n-1) - spline.vecG.at(n-2)) / (tn - tn1);
         GP += (tn - tn1) * spline.vecGamma.at(n-2) / 6.;

         // valeurGSeconde
         //GS =0.;

         // ValeurG
         G = spline.vecG.at(n-1) + (tReduce - tn) * GP;


     } else {
        double err1, err2;
         for (; i0 < n-1; ++i0) {
             const double ti1 = reduceTime(spline.vecThetaEvents.at(i0));
             const double ti2 = reduceTime(spline.vecThetaEvents.at(i0 + 1));
             h = ti2 - ti1;

             if ((tReduce >= ti1) && (tReduce < ti2)) {

                 const double gi1 = spline.vecG.at(i0);
                 const double gi2 = spline.vecG.at(i0 + 1);
                 const double gamma1 = spline.vecGamma.at(i0);
                 const double gamma2 = spline.vecGamma.at(i0 + 1);

                 // ValeurG

                 G = ( (tReduce-ti1)*gi2 + (ti2-tReduce)*gi1 ) /h;
                  // Smoothing part :
                 G -= (1./6.) * ((tReduce-ti1) * (ti2-tReduce)) * ((1.+(tReduce-ti1)/h) * gamma2 + (1.+(ti2-tReduce)/h) * gamma1);

                 err1 = sqrt(spline.vecVarG.at(i0));
                 err2 = sqrt(spline.vecVarG.at(i0 + 1));
                 varG = pow(err1 + ((tReduce-ti1) / (ti2-ti1)) * (err2 - err1) , 2.l);

                 GP = ((gi2-gi1)/h) - (1./6.) * (tReduce-ti1) * (ti2-tReduce) * ((gamma2-gamma1)/h);
                 GP += (1./6.) * ((tReduce-ti1) - (ti2-tReduce)) * ( (1.+(tReduce-ti1)/h) * gamma2 + (1+(ti2-tReduce)/h) * gamma1 );

                 // valeurGSeconde
                 GS = ((tReduce-ti1) * gamma2 + (ti2-tReduce) * gamma1) / h;


                 break;
             }
         }

     }

     // Value slope correction
     GP /=(mSettings.mTmax - mSettings.mTmin);
     GS /= pow(mSettings.mTmax - mSettings.mTmin, 2.);
}

void ModelCurve::valeurs_G_varG_on_i(const MCMCSplineComposante& spline, double& G, double& varG, unsigned long &i)
{
    const double n = spline.vecThetaEvents.size();
    if (i < 0 || i > n-1) {
        G = 0;
        varG = 0;

    } else if (i == 0 || i == n-1) {
        G = spline.vecG.at(i);
        varG = spline.vecVarG.at(i);

    } else if ((i > 0) && (i < n-1)) {

        const double tReduce =  reduceTime(spline.vecThetaEvents.at(i)); //(t - tmin) / (tmax - tmin);

        const double ti1 = reduceTime(spline.vecThetaEvents.at(i));
        const double ti2 = reduceTime(spline.vecThetaEvents.at(i + 1));
        const double h = ti2 - ti1;
        const double gi1 = spline.vecG.at(i);

        // ValeurG
        G =  (ti2-tReduce)*gi1 /h;

        varG = spline.vecVarG.at(i);



    }

}



/**
 * @brief ModelCurve::exportMeanGComposanteToReferenceCurves
 * @param defaultPath
 * @param locale
 * @param csvSep
 * @param step
 */
void ModelCurve::exportMeanGComposanteToReferenceCurves(const PosteriorMeanGComposante pMeanCompoXYZ, const QString& defaultPath, QLocale csvLocale, const QString& csvSep) const
{
    QString filter = tr("CSV (*.csv)");
    QString filename = QFileDialog::getSaveFileName(qApp->activeWindow(),
                                                    tr("Save Ref. Curve as..."),
                                                    defaultPath,
                                                    filter);
    QFile file(filename);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        qDebug()<<"ModelCurve::exportMeanGComposanteToReferenceCurves";

        QList<QStringList> rows;
        QStringList list;
        // 1 -Create the header
        list << "X Axis";
        list << "G";
        list << "err G";
        double xMin = mSettings.mTmin;
        double xMax = mSettings.mTmax;

        const double step = (xMax - xMin)/(pMeanCompoXYZ.vecG.size() - 1);

        rows<<list;
        rows.reserve(pMeanCompoXYZ.vecG.size());

        // 3 - Create Row, with each curve
        //  Create data in row

        csvLocale.setNumberOptions(QLocale::OmitGroupSeparator);
        for (unsigned long i = 0; i < pMeanCompoXYZ.vecG.size(); ++i) {
            const double x = i*step + xMin;
            list.clear();

            list << csvLocale.toString(x);
            // Il doit y avoir au moins trois courbes G, GSup, Ginf et nous exportons G et ErrG
            const double xi = pMeanCompoXYZ.vecG[i]; // G
            const double var_xi =  pMeanCompoXYZ.vecVarG[i];
            list<<csvLocale.toString(xi, 'g', 15);
            list<<csvLocale.toString(sqrt(var_xi), 'g', 15);

            rows<<list;
        }

        // 4 - Save Qlist

        QTextStream output(&file);
        const QString version = qApp->applicationName() + " " + qApp->applicationVersion();
        const QString projectName = tr("Project filename : %1").arg(MainWindow::getInstance()->getNameProject());

        output << "# "+ version + "\r";
        output << "# "+ projectName + "\r";
        output << "# BC/AD \r";//DateUtils::getAppSettingsFormatStr() + "\r";

        for (auto& row : rows) {
            output << row.join(csvSep);
            output << "\r";
        }
        file.close();
    }

}

std::vector<MCMCSpline> ModelCurve::fullRunSplineTrace(const QList<ChainSpecs>& chains)
{
  // Calcul reserve space
    int reserveSize = 0;

    for (const ChainSpecs& chain : chains)
        reserveSize += chain.mRealyAccepted;

    std::vector<MCMCSpline> splineRunTrace(reserveSize);

    int shift = 0;
    int shiftTrace = 0;

    for (const ChainSpecs& chain : chains) {
        // we add 1 for the init
        const int burnAdaptSize = 1 + chain.mIterPerBurn + int (chain.mBatchIndex * chain.mIterPerBatch);
        const int runTraceSize = chain.mRealyAccepted;
        const int firstRunPosition = shift + burnAdaptSize;
        std::copy(mSplinesTrace.begin() + firstRunPosition , mSplinesTrace.begin() + firstRunPosition + runTraceSize , splineRunTrace.begin() + shiftTrace);

        shiftTrace += runTraceSize;
        shift = firstRunPosition +runTraceSize;
    }
    return splineRunTrace;
}
