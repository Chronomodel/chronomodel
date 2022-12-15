/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2022

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
#include <thread>


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
        if (event->type() ==  Event::eBound)
               event->mTheta.mSamplerProposal = MHVariable::eFixe;

        else if (event->type() ==  Event::eDefault) {
                if (mCurveSettings.mTimeType == CurveSettings::eModeFixed) {
                    event->mTheta.mSamplerProposal = MHVariable::eFixe;
                    for (Date &d : event->mDates) {
                        d.mTi.mSamplerProposal = MHVariable::eFixe;
                        d.mSigmaTi.mSamplerProposal = MHVariable::eFixe;
                    }
                } else
                    event->mTheta.mSamplerProposal = MHVariable::eMHAdaptGauss;

        }

        if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed)
            event->mVg.mSamplerProposal = MHVariable::eFixe;

        else if (event->mPointType == Event::eNode)
            event->mVg.mSamplerProposal = MHVariable::eFixe;
        else
            event->mVg.mSamplerProposal = MHVariable::eMHAdaptGauss;

    }

    mLambdaSpline.setName( "lambdaSpline");
    if (mCurveSettings.mLambdaSplineType == CurveSettings::eModeFixed)
        mLambdaSpline.mSamplerProposal = MHVariable::eFixe;
    else
        mLambdaSpline.mSamplerProposal = MHVariable::eMHAdaptGauss;

    mS02Vg.setName("S02Vg");
    if (mCurveSettings.mVarianceType == CurveSettings::eModeFixed)
        mS02Vg.mSamplerProposal = MHVariable::eFixe;
    else
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
        *out << event->mVg;

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
        *in >> e->mVg;

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
            valeurs_G_VarG_GP_GS(t, splineXYZ, g, varG, gp, gs, i0, *this);

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
        for (unsigned c = 0; c < map._row; ++c)  {
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
        for (unsigned c = 0; c < map._row; ++c)  {
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
        event->mVg.updateFormatedTrace();
        event->mVg.generateHistos(chains, fftLen, bandwidth);
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
        if (event->mVg.mSamplerProposal != MHVariable::eFixe)
            event->mVg.generateCorrelations(chains);

    if (mLambdaSpline.mSamplerProposal != MHVariable::eFixe)
        mLambdaSpline.generateCorrelations(chains);

    if (mS02Vg.mSamplerProposal != MHVariable::eFixe)
        mS02Vg.generateCorrelations(chains);
}

void ModelCurve::generateNumericalResults(const QList<ChainSpecs> &chains)
{
    Model::generateNumericalResults(chains);
    for (Event*& event : mEvents) {
        event->mVg.generateNumericalResults(chains);
    }
    mLambdaSpline.generateNumericalResults(chains);

    mS02Vg.generateNumericalResults(chains);
}

void ModelCurve::clearThreshold()
{
    Model::clearThreshold();
   // mThreshold = -1.;
    for (Event*& event : mEvents)
        event->mVg.mThresholdUsed = -1.;

    mLambdaSpline.mThresholdUsed = -1.;
    mS02Vg.mThresholdUsed = -1.;
}

void ModelCurve::generateCredibility(const double& thresh)
{
    Model::generateCredibility(thresh);

    for (Event*& event : mEvents) {
        event->mVg.generateCredibility(mChains, thresh);
    }
    mLambdaSpline.generateCredibility(mChains, thresh);

    mS02Vg.generateCredibility(mChains, thresh);
}

void ModelCurve::generateHPD(const double thresh)
{
    Model::generateHPD(thresh);

    for (Event*& event : mEvents) {
        if (event->type() != Event::eBound) {
            if (event->mVg.mSamplerProposal != MHVariable::eFixe)
                event->mVg.generateHPD(thresh);
        }
    };

    if (mLambdaSpline.mSamplerProposal != MHVariable::eFixe)
        mLambdaSpline.generateHPD(thresh);

    if (mS02Vg.mSamplerProposal != MHVariable::eFixe)
        mS02Vg.generateHPD(thresh);

}

void ModelCurve::clearPosteriorDensities()
{
    Model::clearPosteriorDensities();
    
    for (Event*& event : mEvents) {
        if (event->type() != Event::eBound) {
            event->mVg.mHisto.clear();
            event->mVg.mChainsHistos.clear();
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
            event->mVg.mHPD.clear();
            event->mVg.mCredibility = std::pair<double, double>();
        }
    }
    
    mLambdaSpline.mHPD.clear();
    mLambdaSpline.mCredibility = std::pair<double, double>();

    mS02Vg.mHPD.clear();
    mS02Vg.mCredibility = std::pair<double, double>();
}

void ModelCurve::clearTraces()
{
    Model::clearTraces();
  /*  for (Event*& event : mEvents)
        event->mVg.reset();
*/
    mLambdaSpline.reset();
    mS02Vg.reset();
}


void ModelCurve::setThresholdToAllModel(const double threshold)
{
    Model::setThresholdToAllModel(threshold);
    
    for (Event*& event : mEvents)
        event->mVg.mThresholdUsed = mThreshold;

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

void ModelCurve::valeurs_G_VarG_GP_GS(const double t, const MCMCSplineComposante &spline, double& G, double& varG, double& GP, double& GS, unsigned& i0, const Model &model)
{
    const unsigned long n = spline.vecThetaEvents.size();
    const t_reduceTime tReduce =  model.reduceTime(t);
    const t_reduceTime t1 = model.reduceTime(spline.vecThetaEvents.at(0));
    const t_reduceTime tn = model.reduceTime(spline.vecThetaEvents.at(n-1));
    GP = 0.;
    GS = 0.;
    double h;

     // The first derivative is always constant outside the interval [t1,tn].
     if (tReduce < t1) {
         const t_reduceTime t2 = model.reduceTime(spline.vecThetaEvents.at(1));

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

         const t_reduceTime tn1 = model.reduceTime(spline.vecThetaEvents.at(n-2));

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
             const t_reduceTime ti1 = model.reduceTime(spline.vecThetaEvents.at(i0));
             const t_reduceTime ti2 = model.reduceTime(spline.vecThetaEvents.at(i0 + 1));
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
     GP /=(model.mSettings.mTmax - model.mSettings.mTmin);
     GS /= pow(model.mSettings.mTmax - model.mSettings.mTmin, 2.);
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

        const t_reduceTime tReduce =  reduceTime(spline.vecThetaEvents.at(i)); //(t - tmin) / (tmax - tmin);

        const t_reduceTime ti1 = reduceTime(spline.vecThetaEvents.at(i));
        const t_reduceTime ti2 = reduceTime(spline.vecThetaEvents.at(i + 1));
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


void ModelCurve::memo_PosteriorG_3D(PosteriorMeanG &postG, MCMCSpline &spline, CurveSettings::ProcessType curveType, const int realyAccepted)
{
    const double deg = 180. / M_PI ;
    const bool hasZ = (mCurveSettings.mProcessType == CurveSettings::eProcessTypeVector ||
                       mCurveSettings.mProcessType == CurveSettings::eProcessTypeSpherical ||
                       mCurveSettings.mProcessType == CurveSettings::eProcessType3D);

    auto* curveMap_XInc = &postG.gx.mapG;
    auto* curveMap_YDec = &postG.gy.mapG;
    auto* curveMap_ZF = &postG.gz.mapG;

    const int nbPtsX = curveMap_ZF->column(); // identique à toutes les maps

    const int nbPtsY_XInc = curveMap_XInc->row();
    const int nbPtsY_YDec = curveMap_YDec->row();
    const int nbPtsY_ZF = curveMap_ZF->row();

    const double ymin_XInc = curveMap_XInc->minY();
    const double ymax_XInc = curveMap_XInc->maxY();

    const double ymin_YDec = curveMap_YDec->minY();
    const double ymax_YDec = curveMap_YDec->maxY();

    const double ymin_ZF = curveMap_ZF->minY();
    const double ymax_ZF = curveMap_ZF->maxY();

    const double stepT = (mSettings.mTmax - mSettings.mTmin) / (nbPtsX - 1);
    const double stepY_XInc = (ymax_XInc - ymin_XInc) / (nbPtsY_XInc - 1);
    const double stepY_YDec = (ymax_YDec - ymin_YDec) / (nbPtsY_YDec - 1);
    const double stepY_ZF = (ymax_ZF - ymin_ZF) / (nbPtsY_ZF - 1);

    // 2 - Variables temporaires
    // référence sur variables globales
    std::vector<double> &vecVarG_XInc = postG.gx.vecVarG;
    std::vector<double> &vecVarG_YDec = postG.gy.vecVarG;
    std::vector<double> &vecVarG_ZF = postG.gz.vecVarG;
    // Variables temporaires
    // erreur inter spline
    std::vector<double> &vecVarianceG_XInc = postG.gx.vecVarianceG;
    std::vector<double> &vecVarianceG_YDec = postG.gy.vecVarianceG;
    std::vector<double> &vecVarianceG_ZF = postG.gz.vecVarianceG;
    // erreur intra spline
    std::vector<double> &vecVarErrG_XInc = postG.gx.vecVarErrG;
    std::vector<double> &vecVarErrG_YDec = postG.gy.vecVarErrG;
    std::vector<double> &vecVarErrG_ZF = postG.gz.vecVarErrG;

    //Pointeur sur tableau
    std::vector<double>::iterator itVecG_XInc = postG.gx.vecG.begin();
    std::vector<double>::iterator itVecGP_XInc = postG.gx.vecGP.begin();
    std::vector<double>::iterator itVecGS_XInc = postG.gx.vecGS.begin();

    std::vector<double>::iterator itVecG_YDec = postG.gy.vecG.begin();
    std::vector<double>::iterator itVecGP_YDec = postG.gy.vecGP.begin();
    std::vector<double>::iterator itVecGS_YDec = postG.gy.vecGS.begin();

    std::vector<double>::iterator itVecG_ZF = postG.gz.vecG.begin();
    std::vector<double>::iterator itVecGP_ZF = postG.gz.vecGP.begin();
    std::vector<double>::iterator itVecGS_ZF = postG.gz.vecGS.begin();

    // Variables temporaires
    // erreur inter spline
    std::vector<double>::iterator itVecVarianceG_XInc = postG.gx.vecVarianceG.begin();
    std::vector<double>::iterator itVecVarianceG_YDec = postG.gy.vecVarianceG.begin();
    std::vector<double>::iterator itVecVarianceG_ZF = postG.gz.vecVarianceG.begin();
    // erreur intra spline
    std::vector<double>::iterator itVecVarErrG_XInc = postG.gx.vecVarErrG.begin();
    std::vector<double>::iterator itVecVarErrG_YDec = postG.gy.vecVarErrG.begin();
    std::vector<double>::iterator itVecVarErrG_ZF = postG.gz.vecVarErrG.begin();

    // inter derivate variance

    double t;
    double gx, gpx, gsx, varGx = 0;
    double gy, gpy, gsy, varGy = 0;
    double gz, gpz, gsz, varGz = 0;


    double n = realyAccepted;
    double  prevMeanG_XInc, prevMeanG_YDec, prevMeanG_ZF;

    const double k = 3.; // Le nombre de fois sigma G, pour le calcul de la densité
    //double a, b, surfG;

    int  idxYErrMin, idxYErrMax;

    // 3 - Calcul pour la composante
    unsigned i0 = 0; // tIdx étant croissant, i0 permet de faire la recherche à l'indice du temps précedent
    for (int idxT = 0; idxT < nbPtsX ; ++idxT) {
        t = (double)idxT * stepT + mSettings.mTmin ;
        valeurs_G_VarG_GP_GS(t, spline.splineX, gx, varGx, gpx, gsx, i0, *this);
        valeurs_G_VarG_GP_GS(t, spline.splineY, gy, varGy, gpy, gsy, i0, *this);

       // if (hasZ)
            valeurs_G_VarG_GP_GS(t, spline.splineZ, gz, varGz, gpz, gsz, i0, *this);

        // Conversion IDF
        if (curveType == CurveSettings::eProcessTypeVector ||  curveType == CurveSettings::eProcessTypeSpherical) {
            const double F = sqrt(pow(gx, 2.) + pow(gy, 2.) + pow(gz, 2.));
            const double Inc = asin(gz / F);
            const double Dec = atan2(gy, gx);

            const double ErrF = sqrt((varGx + varGy + varGz)/3.);

            const double ErrI = ErrF / F ;
            const double ErrD = ErrF / (F * cos(Inc)) ;

            gx = Inc * deg;
            gy = Dec * deg;
            gz = F;

            varGx = ErrI * deg;
            varGy = ErrD * deg;
            varGz = ErrF;
        }


        // -- Calcul Mean on XInc
        prevMeanG_XInc = *itVecG_XInc;
        *itVecG_XInc +=  (gx - prevMeanG_XInc)/n;

        *itVecGP_XInc +=  (gpx - *itVecGP_XInc)/n;
        *itVecGS_XInc +=  (gsx - *itVecGS_XInc)/n;
        // erreur inter spline
        *itVecVarianceG_XInc +=  (gx - prevMeanG_XInc)*(gx - *itVecG_XInc);
        // erreur intra spline
        *itVecVarErrG_XInc += (varGx - *itVecVarErrG_XInc) / n  ;

        ++itVecG_XInc;
        ++itVecGP_XInc;
        ++itVecGS_XInc;
        ++itVecVarianceG_XInc;
        ++itVecVarErrG_XInc;


        // -- Calcul map on XInc

        auto stdGx = sqrt(varGx);

        // Ajout densité erreur sur Y
        /* il faut utiliser un pas de grille et le coefficient dans la grille dans l'intervalle [a,b] pour N(mu, sigma) est égale à la différence 1/2*(erf((b-mu)/(sigma*sqrt(2)) - erf((a-mu)/(sigma*sqrt(2))
         * https://en.wikipedia.org/wiki/Error_function
         */
        idxYErrMin = inRange( 0, int((gx - k*stdGx - ymin_XInc) / stepY_XInc), nbPtsY_XInc-1);
        idxYErrMax = inRange( 0, int((gx + k*stdGx - ymin_XInc) / stepY_XInc), nbPtsY_XInc-1);

        if (idxYErrMin == idxYErrMax && idxYErrMin > 0 && idxYErrMax < nbPtsY_XInc-1) {
#ifdef DEBUG
            if ((curveMap_XInc->row()*idxT + idxYErrMin) < (curveMap_XInc->row()*curveMap_XInc->column()))
                (*curveMap_XInc)(idxT, idxYErrMin) = curveMap_XInc->at(idxYErrMin, idxYErrMin) + 1; // correction à faire dans finalize() + 1./nbIter;
            else
                qDebug()<<"pb in MCMCLoopCurve::memo_PosteriorG";
#else
            (*curveMap_XInc)(idxT, idxYErrMin) = curveMap_XInc->at(idxT, idxYErrMin) + 1.; // correction à faire dans finalize/nbIter ;
#endif

            curveMap_XInc->max_value = std::max(curveMap_XInc->max_value, curveMap_XInc->at(idxT, idxYErrMin));


        } else if (0 <= idxYErrMin && idxYErrMax < nbPtsY_XInc) {
            double* ptr_Ymin = curveMap_XInc->ptr_at(idxT, idxYErrMin);
            double* ptr_Ymax = curveMap_XInc->ptr_at(idxT, idxYErrMax);

            int idErr = idxYErrMin;
            for (double* ptr_idErr = ptr_Ymin; ptr_idErr <= ptr_Ymax; ptr_idErr++) {
                double a = (idErr - 0.5) * stepY_XInc + ymin_XInc;
                double b = (idErr + 0.5) * stepY_XInc + ymin_XInc;
                double surfG = diff_erf(a, b, gx, stdGx );// correction à faire dans finalyze /nbIter;
#ifdef DEBUG
                *ptr_idErr = (*ptr_idErr) + surfG;
#else
                //curveMap(idxT, idxY) = curveMap.at(idxT, idxY) + coefG/(double)(trace.size() * 1);
                *ptr_idErr = (*ptr_idErr) + surfG;
#endif

                curveMap_XInc->max_value = std::max(curveMap_XInc->max_value, *ptr_idErr);

                idErr++;
            }
        }



        // -- Calcul Mean on YDec
        prevMeanG_YDec = *itVecG_YDec;
        *itVecG_YDec +=  (gy - prevMeanG_YDec)/n;

        *itVecGP_YDec +=  (gpy - *itVecGP_YDec)/n;
        *itVecGS_YDec +=  (gsy - *itVecGS_YDec)/n;
        // erreur inter spline
        *itVecVarianceG_YDec +=  (gy - prevMeanG_YDec)*(gy - *itVecG_YDec);
        // erreur intra spline
        *itVecVarErrG_YDec += (varGy - *itVecVarErrG_YDec) / n  ;

        ++itVecG_YDec;
        ++itVecGP_YDec;
        ++itVecGS_YDec;
        ++itVecVarianceG_YDec;
        ++itVecVarErrG_YDec;

        // -- Calcul map on YDec

        auto stdGy = sqrt(varGy);

        // Ajout densité erreur sur Y
        /* Il faut utiliser un pas de grille et le coefficient dans la grille dans l'intervalle [a,b] pour N(mu, sigma) est égale à la différence 1/2*(erf((b-mu)/(sigma*sqrt(2)) - erf((a-mu)/(sigma*sqrt(2))
        * https://en.wikipedia.org/wiki/Error_function
        */
        idxYErrMin = inRange( 0, int((gy - k*stdGy - ymin_YDec) / stepY_YDec), nbPtsY_YDec -1);
        idxYErrMax = inRange( 0, int((gy + k*stdGy - ymin_YDec) / stepY_YDec), nbPtsY_YDec -1);

        if (idxYErrMin == idxYErrMax && idxYErrMin > 0 && idxYErrMax < nbPtsY_YDec-1) {
#ifdef DEBUG
            if ((curveMap_YDec->row()*idxT + idxYErrMin) < (curveMap_YDec->row()*curveMap_YDec->column()))
                (*curveMap_YDec)(idxT, idxYErrMin) = curveMap_YDec->at(idxYErrMin, idxYErrMin) + 1;
            else
                qDebug()<<"pb in MCMCLoopCurve::memo_PosteriorG";
#else
            (*curveMap_YDec)(idxT, idxYErrMin) = curveMap_YDec->at(idxT, idxYErrMin) + 1.; // correction à faire dans finalize/nbIter ;
#endif

            curveMap_YDec->max_value = std::max(curveMap_YDec->max_value, curveMap_YDec->at(idxT, idxYErrMin));


        } else if (0 <= idxYErrMin && idxYErrMax < nbPtsY_YDec) {
            double* ptr_Ymin = curveMap_YDec->ptr_at(idxT, idxYErrMin);
            double* ptr_Ymax = curveMap_YDec->ptr_at(idxT, idxYErrMax);

            int idErr = idxYErrMin;
            for (double* ptr_idErr = ptr_Ymin; ptr_idErr <= ptr_Ymax; ptr_idErr++) {
                double a = (idErr - 0.5) * stepY_YDec + ymin_YDec;
                double b = (idErr + 0.5) * stepY_YDec + ymin_YDec;
                double surfG = diff_erf(a, b, gy, stdGy );
#ifdef DEBUG
                *ptr_idErr = (*ptr_idErr) + surfG;
#else
                //curveMap(idxT, idxY) = curveMap.at(idxT, idxY) + coefG/(double)(trace.size() * 1);
                *ptr_idErr = (*ptr_idErr) + surfG;
#endif

                curveMap_YDec->max_value = std::max(curveMap_YDec->max_value, *ptr_idErr);

                idErr++;
            }
        }


        if (hasZ) {

            // -- Calcul Mean on ZF
            prevMeanG_ZF = *itVecG_ZF;
            *itVecG_ZF +=  (gz - prevMeanG_ZF)/n;

            *itVecGP_ZF +=  (gpz - *itVecGP_ZF)/n;
            *itVecGS_ZF +=  (gsz - *itVecGS_ZF)/n;
            // erreur inter spline
            *itVecVarianceG_ZF +=  (gz - prevMeanG_ZF)*(gz - *itVecG_ZF);
            // erreur intra spline
            *itVecVarErrG_ZF += (varGz - *itVecVarErrG_ZF) / n  ;

            ++itVecG_ZF;
            ++itVecGP_ZF;
            ++itVecGS_ZF;
            ++itVecVarianceG_ZF;
            ++itVecVarErrG_ZF;


            // -- Calcul map on ZF

            // curveMap = curveMap_ZF;//postG.gz.mapG;
            const auto stdGz = sqrt(varGz);

            // ajout densité erreur sur Y
            /* il faut utiliser un pas de grille et le coefficient dans la grille dans l'intervalle [a,b] pour N(mu, sigma) est égale à la différence 1/2*(erf((b-mu)/(sigma*sqrt(2)) - erf((a-mu)/(sigma*sqrt(2))
             * https://en.wikipedia.org/wiki/Error_function
             */
            idxYErrMin = inRange( 0, int((gz - k*stdGz - ymin_ZF) / stepY_ZF), nbPtsY_ZF-1);
            idxYErrMax = inRange( 0, int((gz + k*stdGz - ymin_ZF) / stepY_ZF), nbPtsY_ZF-1);

            if (idxYErrMin == idxYErrMax && idxYErrMin > 0 && idxYErrMax < nbPtsY_ZF-1) {
#ifdef DEBUG
                if ((curveMap_ZF->row()*idxT + idxYErrMin) < (curveMap_ZF->row()*curveMap_ZF->column()))
                    (*curveMap_ZF)(idxT, idxYErrMin) = curveMap_ZF->at(idxYErrMin, idxYErrMin) + 1;
                else
                    qDebug()<<"pb in MCMCLoopCurve::memo_PosteriorG";
#else
                (*curveMap_ZF)(idxT, idxYErrMin) = curveMap_ZF->at(idxT, idxYErrMin) + 1.;
#endif

                curveMap_ZF->max_value = std::max(curveMap_ZF->max_value, curveMap_ZF->at(idxT, idxYErrMin));


            } else if (0 <= idxYErrMin && idxYErrMax < nbPtsY_ZF) {
                double* ptr_Ymin = curveMap_ZF->ptr_at(idxT, idxYErrMin);
                double* ptr_Ymax = curveMap_ZF->ptr_at(idxT, idxYErrMax);

                int idErr = idxYErrMin;
                for (double* ptr_idErr = ptr_Ymin; ptr_idErr <= ptr_Ymax; ptr_idErr++) {
                    double a = (idErr - 0.5) * stepY_ZF + ymin_ZF;
                    double b = (idErr + 0.5) * stepY_ZF + ymin_ZF;
                    double surfG = diff_erf(a, b, gz, stdGz );
#ifdef DEBUG
                    *ptr_idErr = (*ptr_idErr) + surfG;
#else
                    //curveMap(idxT, idxY) = curveMap.at(idxT, idxY) + coefG/(double)(trace.size() * 1);
                    *ptr_idErr = (*ptr_idErr) + surfG;
#endif

                    curveMap_ZF->max_value = std::max(curveMap_ZF->max_value, *ptr_idErr);

                    idErr++;
                }
            }


        }


    }


    int tIdx = 0;
    for (auto& vVarG : vecVarG_XInc) {
        vVarG = vecVarianceG_XInc.at(tIdx)/ n + vecVarErrG_XInc.at(tIdx);
        ++tIdx;
    }
    tIdx = 0;
    for (auto& vVarG : vecVarG_YDec) {
        vVarG = vecVarianceG_YDec.at(tIdx)/ n + vecVarErrG_YDec.at(tIdx);
        ++tIdx;
    }
    tIdx = 0;
    for (auto& vVarG : vecVarG_ZF) {
        vVarG = vecVarianceG_ZF.at(tIdx)/ n + vecVarErrG_ZF.at(tIdx);
        ++tIdx;
    }
}

void ModelCurve::memo_PosteriorG(PosteriorMeanGComposante& postGCompo, MCMCSplineComposante& splineComposante, const int realyAccepted)
{
    CurveMap& curveMap = postGCompo.mapG;
    const int nbPtsX = curveMap.column();
    const int nbPtsY = curveMap.row();

    const double ymin = curveMap.minY();
    const double ymax = curveMap.maxY();

    const double stepT = (mSettings.mTmax - mSettings.mTmin) / (nbPtsX - 1);
    const double stepY = (ymax - ymin) / (nbPtsY - 1);

    // 2 - Variables temporaires
    // référence sur variables globales
    std::vector<double>& vecVarG = postGCompo.vecVarG;
    // Variables temporaires
    // erreur inter spline
    std::vector<double>& vecVarianceG = postGCompo.vecVarianceG;
    // erreur intra spline
    std::vector<double>& vecVarErrG = postGCompo.vecVarErrG;

    //Pointeur sur tableau
    std::vector<double>::iterator itVecG = postGCompo.vecG.begin();
    std::vector<double>::iterator itVecGP = postGCompo.vecGP.begin();
    std::vector<double>::iterator itVecGS = postGCompo.vecGS.begin();
    //std::vector<long double>::iterator itVecVarG = posteriorMeanCompo.vecVarG.begin();
    // Variables temporaires
    // erreur inter spline
    std::vector<double>::iterator itVecVarianceG = postGCompo.vecVarianceG.begin();
    // erreur intra spline
    std::vector<double>::iterator itVecVarErrG = postGCompo.vecVarErrG.begin();

    // inter derivate variance
    //std::vector<double>::iterator itVecVarianceGP = postGCompo.vecVarGP.begin();

    double t, g, gp, gs, varG, stdG;
    g = 0.;
    gp = 0;
    varG = 0;
    gs = 0;

    double n = realyAccepted;
    double  prevMeanG; //, prevMeanGP;

    const double k = 3.; // Le nombre de fois sigma G, pour le calcul de la densité
    double a, b, surfG;

    int  idxYErrMin, idxYErrMax;

    // 3 - calcul pour la composante
    unsigned i0 = 0; // tIdx étant croissant, i0 permet de faire la recherche à l'indice du temps précedent
    for (int idxT = 0; idxT < nbPtsX ; ++idxT) {
        t = (double)idxT * stepT + mSettings.mTmin ;
        valeurs_G_VarG_GP_GS(t, splineComposante, g, varG, gp, gs, i0, *this);

       /*const auto variableType = mCurveSettings.mVariableType;

        switch (variableType) {
        case CurveSettings::eVariableTypeInclination:
            gx = g;
            varGx = varG;
            break;
        case CurveSettings::eVariableTypeDeclination:
            gx = g;
            varGx = varG;
            break;
        default:
            gx = g;
            varGx = varG;
            break;
        }
*/

        // -- calcul Mean
        prevMeanG = *itVecG;
        *itVecG +=  (g - prevMeanG)/n;

        *itVecGP +=  (gp - *itVecGP)/n;
        *itVecGS +=  (gs - *itVecGS)/n;
        // erreur inter spline
        *itVecVarianceG +=  (g - prevMeanG)*(g - *itVecG);
        // erreur intra spline
        *itVecVarErrG += (varG - *itVecVarErrG) / n  ;

        // inter derivate variance
        //*itVecVarianceGP +=  (gp - prevMeanGP)*(gp - *itVecGP);

        ++itVecG;
        ++itVecGP;
        ++itVecGS;
        ++itVecVarianceG;
        ++itVecVarErrG;


        // -- calcul map

        stdG = sqrt(varG);

        // Ajout densité erreur sur Y
        /* il faut utiliser un pas de grille et le coefficient dans la grille dans l'intervalle [a,b] pour N(mu, sigma) est égale à la différence 1/2*(erf((b-mu)/(sigma*sqrt(2)) - erf((a-mu)/(sigma*sqrt(2))
         * https://en.wikipedia.org/wiki/Error_function
         */
        idxYErrMin = inRange( 0, int((g - k*stdG - ymin) / stepY), nbPtsY-1);
        idxYErrMax = inRange( 0, int((g + k*stdG - ymin) / stepY), nbPtsY-1);

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

        } else if (0 <= idxYErrMin && idxYErrMax < nbPtsY) {
            double* ptr_Ymin = curveMap.ptr_at(idxT, idxYErrMin);
            double* ptr_Ymax = curveMap.ptr_at(idxT, idxYErrMax);

            int idErr = idxYErrMin;
            for (double* ptr_idErr = ptr_Ymin; ptr_idErr <= ptr_Ymax; ptr_idErr++) {
                a = (idErr - 0.5)*stepY + ymin;
                b = (idErr + 0.5)*stepY + ymin;
                surfG = diff_erf(a, b, g, stdG );// correction à faire dans finalyze /nbIter;
#ifdef DEBUG

                *ptr_idErr = (*ptr_idErr) + surfG;

#else
                //curveMap(idxT, idxY) = curveMap.at(idxT, idxY) + coefG/(double)(trace.size() * 1);
                *ptr_idErr = (*ptr_idErr) + surfG;
#endif

                curveMap.max_value = std::max(curveMap.max_value, *ptr_idErr);

                idErr++;
            }
        }


    }
    int tIdx = 0;
    for (auto& vVarG : vecVarG) {
        vVarG = vecVarianceG.at(tIdx)/ n + vecVarErrG.at(tIdx);
        ++tIdx;
    }
}
