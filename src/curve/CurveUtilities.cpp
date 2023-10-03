/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2023

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

#include "CurveUtilities.h"
#include "Model.h"

#include <algorithm>
#include <iostream>


inline decltype(Event::mThetaReduced) diffX (Event* e0, Event*e1) {return (e1->mThetaReduced - e0->mThetaReduced);}


std::vector<t_reduceTime> calculVecH(const QList<Event *> &event)
{
    std::vector<double> result (event.size()-1);
    std::transform(event.begin(), event.end()-1, event.begin()+1 , result.begin(), diffX);

#ifdef DEBUG
    int i =0;
    for (auto &&r :result) {
        if (r <= 1.E-10) {
            qDebug()<< "[CurveUtilities::calculVecH] diff Theta r <= 1.E-10 "<< (double)event.at(i)->mThetaReduced<< (double)event.at(i+1)->mThetaReduced;

        }
        ++i;
    }
#endif
    return result;
}

/*
std::vector<t_reduceTime> calculVecH(const std::vector<t_reduceTime>& vec)
{
    // 2 codes possible

    //std::vector<double> result(vec.size() - 1);
    //std::transform(vec.begin(), vec.end()-1, vec.begin()+1,  result.begin(),  [](int v, int v1) {return v1-v; } );


    std::vector<t_reduceTime>result(vec.size());
    std::adjacent_difference (vec.begin(), vec.end(), result.begin());
    result.erase(result.begin());

    return result;
}
*/
// --------------- Function with list of double value

Matrix2D calculMatR(const std::vector<t_reduceTime> &rVecH)
{
    // Calcul de la matrice R, de dimension (n-2) x (n-2) contenue dans une matrice n x n
    // Par exemple pour n = 5 :
    // 0 0 0 0 0
    // 0 X X X 0
    // 0 X X X 0
    // 0 X X X 0
    // 0 0 0 0 0

    // vecH est de dimension n-1
    //const std::vector<t_reduceTime> rVecH = calculVecH(vec);
    const unsigned long n = rVecH.size() +1;

    // matR est de dimension n-2 x n-2, mais contenue dans une matrice nxn
    Matrix2D matR = initMatrix2D(n, n);
    // On parcourt n-2 valeurs :
    /* pHd : code simplified
    for (int i = 1; i < n-1; ++i) {
        matR[i][i] = (vecH[i-1] + vecH[i]) / 3.;
        // Si on est en n-2 (dernière itération), on ne calcule pas les valeurs de part et d'autre de la diagonale (termes symétriques)
        if (i < n-2) {
            matR[i][i+1] = vecH[i] / 6.;
            matR[i+1][i] = vecH[i] / 6.;
        }
    }
    */
    for ( unsigned long i = 1; i < n-2; ++i) {
        matR[i][i] = (rVecH[i-1] + rVecH[i]) / 3.;
        matR[i][i+1] = rVecH[i] / 6.;
        matR[i+1][i] = rVecH[i] / 6.;
    }
    // Si on est en n-2 (dernière itération), on ne calcule pas les valeurs de part et d'autre de la diagonale (termes symétriques)
   matR[n-2][n-2] = (rVecH[n-2-1] + rVecH[n-2]) / 3.;

    return matR;
}

// Dans RenCurve procedure Calcul_Mat_Q_Qt_R ligne 55
Matrix2D calculMatQ(const std::vector<t_reduceTime>& rVecH)
{
    // Calcul de la matrice Q, de dimension n x (n-2) contenue dans une matrice n x n
    // Les 1ère et dernière colonnes sont nulles
    // Par exemple pour n = 5 :
    // 0 X 0 0 0
    // 0 X X X 0
    // 0 X X X 0
    // 0 X X X 0
    // 0 0 0 X 0

    // vecH est de dimension n-1
    const size_t n = rVecH.size() + 1;

    // matQ est de dimension n x n-2, mais contenue dans une matrice nxn
    Matrix2D matQ = initMatrix2D(n, n);
    // On parcourt n-2 valeurs :
    for (size_t i = 1; i < n-1; ++i) {
        matQ[i-1][i] = 1. / rVecH[i-1];
        matQ[i][i] = -((1./rVecH[i-1]) + (1./rVecH[i]));
        matQ[i+1][i] = 1. / rVecH[i];

#ifdef DEBUG
        if (rVecH.at(i)<=0)
            throw "calculMatQ vecH <=0 ";
#endif
    }
    // pHd : ici la vrai forme est une matrice de dimension n x (n-2), de bande k=1; les termes diagonaux sont négatifs
    // Les 1ère et dernière colonnes sont nulles
    // Par exemple pour n = 5 :
    // 0 +X  0  0 0
    // 0 -X +a  0 0
    // 0 +a -X +b 0
    // 0  0 +b -X 0
    // 0  0  0 +X 0



    return matQ;
}
/**
 * La création de la matrice diagonale des erreurs est nécessaire à chaque mise à jour de :
 * - Theta event : qui peut engendrer un nouvel ordonnancement des events (definitionNoeuds)
 * - VG event : qui intervient directement dans le calcul de W
 */
std::vector<double> createDiagWInv(const QList<Event*> &events)
{
    std::vector<double> diagWInv (events.size());
    std::transform(events.begin(), events.end(), diagWInv.begin(), [](Event* ev) {return 1/ev->mW;});

    return diagWInv;
}

#pragma mark Init vectors et matrix


// can be replace with #include <functional>   // std::greater
// unused
bool sortItems(const double& a, const double& b)
{
    return (a < b);
}

void display(const std::vector<double>& v)
{
    for (std::vector<double>::const_iterator it=v.begin(); it!=v.end(); ++it) {
        std::cout << *it << ' ' ;
    }
    std::cout << std::endl;
}


PosteriorMeanG conversionIDF(const std::vector<double>& vecGx, const std::vector<double>& vecGy, const std::vector<double>& vecGz, const std::vector<double>& vecGxErr, const std::vector<double> &vecGyErr, const std::vector<double> &vecGzErr)
{
    const double deg = 180. / M_PI ;
    auto n = vecGx.size();
    PosteriorMeanG res;
    res.gx.vecG.resize(n);
    res.gx.vecVarG.resize(n);
    res.gy.vecG.resize(n);
    res.gy.vecVarG.resize(n);
    res.gz.vecG.resize(n);
    res.gz.vecVarG.resize(n);

    for (unsigned long j = 0; j < n ; ++j) {
        const double& Gx = vecGx.at(j);

        const double& Gy = vecGy.at(j);
        const double& Gz = vecGz.at(j);

        const double F = sqrt(pow(Gx, 2.) + pow(Gy, 2.) + pow(Gz, 2.));
        const double Inc = asin(Gz / F);
        const double Dec = atan2(Gy, Gx); // angleD(Gx, Gy);
        // U_cmt_change_repere , ligne 470
        // sauvegarde des erreurs sur chaque paramètre  - on convertit en degrès pour I et D
        // Calcul de la boule d'erreur moyenne par moyenne quadratique loigne 464
       /*   ErrGx:=Tab_parametrique[iJ].ErrGx;
          ErrGy:=Tab_parametrique[iJ].ErrGy;
          ErrGz:=Tab_parametrique[iJ].ErrGz;
          ErrIDF:=sqrt((sqr(ErrGx)+sqr(ErrGy)+sqr(ErrGz))/3);
        */


        const double ErrIDF = sqrt((pow(vecGxErr.at(j), 2.) +pow(vecGyErr.at(j), 2.) +pow(vecGzErr.at(j), 2.))/3.);

        const double ErrI = ErrIDF / F ;
        const double ErrD = ErrIDF / (F * cos(Inc)) ;

       /* long double ErrI = Gz+ErrIDF ; // dans l'espace 3D, l'enveloppe supérieure
        ErrI = abs(asin(ErrIDF/F) - Inc); // pour retrouver la différence

       // long double ErrD = Gz+ErrIDF/F / (F * cos(Inc))
       */
        res.gx.vecG[j] = std::move(Inc * deg);
        res.gx.vecVarG[j] = std::move(ErrI* deg);


        res.gy.vecG[j] = std::move(Dec * deg);
        res.gy.vecVarG[j] = std::move(ErrD* deg);

        res.gz.vecG[j] = std::move(F);
        res.gz.vecVarG[j] = std::move(ErrIDF);

    }

    auto gmaxF = sqrt(pow(res.gx.mapG.maxY(), 2.) + pow(res.gy.mapG.maxY(), 2.) + pow(res.gz.mapG.maxY(), 2.));
    auto gFMax = asin(res.gz.mapG.maxY() / gmaxF);
    const double gIncMax = asin(res.gz.mapG.maxY() / gmaxF);
    const double gDecMax = atan2(res.gy.mapG.maxY(),res.gx.mapG.maxY());

    auto gminF = sqrt(pow(res.gx.mapG.minY(), 2.) + pow(res.gy.mapG.minY(), 2.) + pow(res.gz.mapG.minY(), 2.));
    auto gFMin = asin(res.gz.mapG.minY() / gminF);
    const double gIncMin = asin(res.gz.mapG.minY() / gminF);
    const double gDecMin = atan2(res.gy.mapG.minY(),res.gx.mapG.minY());


    res.gx.mapG.setRangeY(gIncMin, gIncMax);
    res.gy.mapG.setRangeY(gDecMin, gDecMax);
    res.gz.mapG.setRangeY(gFMin, gFMax);

    return res;
}

void conversionIDF (PosteriorMeanG& G)
{
   //PosteriorMeanG res = conversionIDF(G.gx.vecG, G.gy.vecG, G.gz.vecG, G.gx.vecVarG, G.gy.vecVarG, G.gz.vecVarG );
  // PosteriorMeanG res = G; // on peut utiliser le constructeur de la fonction par copie


   const double deg = 180. / M_PI ;

   //PosteriorMeanG& res = G;
   auto& vecGx = G.gx.vecG;
   auto& vecGy = G.gy.vecG;
   auto& vecGz = G.gz.vecG;

   auto& vecGxErr = G.gx.vecVarG;
   auto& vecGyErr = G.gy.vecVarG;
   auto& vecGzErr = G.gz.vecVarG;

   const unsigned long n = vecGx.size();
  /* res.gx.vecG.resize(n);
   res.gx.vecVarG.resize(n);
   res.gy.vecG.resize(n);
   res.gy.vecVarG.resize(n);
   res.gz.vecG.resize(n);
   res.gz.vecVarG.resize(n);*/

   for (unsigned long j = 0; j < n ; ++j) {
       const double& Gx = vecGx.at(j);

       const double& Gy = vecGy.at(j);
       const double& Gz = vecGz.at(j);

       const double F = sqrt(pow(Gx, 2.) + pow(Gy, 2.) + pow(Gz, 2.));
       const double Inc = asin(Gz / F);
       const double Dec = atan2(Gy, Gx); // angleD(Gx, Gy);
       // U_cmt_change_repere , ligne 470
       // sauvegarde des erreurs sur chaque paramètre  - on convertit en degrès pour I et D
       // Calcul de la boule d'erreur moyenne par moyenne quadratique loigne 464
      /*   ErrGx:=Tab_parametrique[iJ].ErrGx;
         ErrGy:=Tab_parametrique[iJ].ErrGy;
         ErrGz:=Tab_parametrique[iJ].ErrGz;
         ErrIDF:=sqrt((sqr(ErrGx)+sqr(ErrGy)+sqr(ErrGz))/3);
       */


       const double ErrIDF = sqrt((pow(vecGxErr.at(j), 2.) + pow(vecGyErr.at(j), 2.) + pow(vecGzErr.at(j), 2.))/3.);

       const double ErrI = ErrIDF / F ;
       const double ErrD = ErrIDF / (F * cos(Inc)) ;

      /* long double ErrI = Gz+ErrIDF ; // dans l'espace 3D, l'enveloppe supérieure
       ErrI = abs(asin(ErrIDF/F) - Inc); // pour retrouver la différence

      // long double ErrD = Gz+ErrIDF/F / (F * cos(Inc))
      */
       G.gx.vecG[j] = std::move(Inc * deg);
       G.gx.vecVarG[j] = std::move(ErrI* deg);


       G.gy.vecG[j] = std::move(Dec * deg);
       G.gy.vecVarG[j] = std::move(ErrD* deg);

       G.gz.vecG[j] = std::move(F);
       G.gz.vecVarG[j] = std::move(ErrIDF);

   }

   // Conversion of the map
   // 1 - nouveau extrenum
   const double gzFmax = sqrt(pow(G.gx.mapG.maxY(), 2.) + pow(G.gy.mapG.maxY(), 2.) + pow(G.gz.mapG.maxY(), 2.));
   const double gxIncMax = asin(G.gz.mapG.maxY() / gzFmax);
   const double gyDecMax = atan2(G.gy.mapG.maxY(), G.gx.mapG.maxY());

   const double gzFmin = sqrt(pow(G.gx.mapG.minY(), 2.) + pow(G.gy.mapG.minY(), 2.) + pow(G.gz.mapG.minY(), 2.));
   const double gxIncMin = asin(G.gz.mapG.minY() / gzFmin);
   const double gyDecMin = atan2(G.gy.mapG.minY(), G.gx.mapG.minY());


   G.gx.mapG.setRangeY(gxIncMin * deg, gxIncMax * deg);
   G.gy.mapG.setRangeY(gyDecMin * deg, gyDecMax * deg);
   G.gz.mapG.setRangeY(std::move(gzFmin), std::move(gzFmax));




   // -----------------
/*   G.gx.vecG = std::move(res.gx.vecG);
   G.gy.vecG = std::move(res.gy.vecG);
   G.gz.vecG = std::move(res.gz.vecG);

   G.gx.vecVarG = std::move(res.gx.vecVarG);
   G.gy.vecVarG = std::move(res.gy.vecVarG);
   G.gz.vecVarG = std::move(res.gz.vecVarG);

   G.gx.mapG = std::move(res.gx.mapG);
   G.gy.mapG = std::move(res.gy.mapG);
   G.gz.mapG = std::move(res.gz.mapG);*/
}

/**
 * @brief conversionID identic to conversionIDF
 * @param vecGx
 * @param vecGy
 * @param vecGz
 * @param vecGErr
 * @return
 */
PosteriorMeanG conversionID(const std::vector<double>& vecGx, const std::vector<double>& vecGy, const std::vector<double>& vecGz, const std::vector<double>& vecGxErr, const std::vector<double>& vecGyErr, const std::vector<double>& vecGzErr)
{
   return conversionIDF(vecGx, vecGy, vecGz, vecGxErr, vecGyErr, vecGzErr);
}

void conversionID (PosteriorMeanG& G)
{
    PosteriorMeanG res = conversionID(G.gx.vecG, G.gy.vecG, G.gz.vecG, G.gx.vecVarG, G.gy.vecVarG, G.gz.vecVarG );
    G.gx.vecG = std::move(res.gx.vecG);
    G.gy.vecG = std::move(res.gy.vecG);
    G.gz.vecG = std::move(res.gz.vecG);

    G.gx.vecVarG = std::move(res.gx.vecVarG);
    G.gy.vecVarG = std::move(res.gy.vecVarG);
    G.gz.vecVarG = std::move(res.gz.vecVarG);
}

// Obsolete

QDataStream &operator<<( QDataStream& stream, const MCMCSplineComposante& splineComposante )
{
    stream << (quint32) splineComposante.vecThetaReduced.size();
    for (auto& v : splineComposante.vecThetaReduced)
        stream << (double)v;

    stream << (quint32) splineComposante.vecG.size();
    for (auto& v : splineComposante.vecG)
        stream << (double)v;

    stream << (quint32) splineComposante.vecGamma.size();
    for (auto& v : splineComposante.vecGamma)
        stream << (double)v;

    stream << (quint32) splineComposante.vecVarG.size();
    for (auto& v : splineComposante.vecVarG)
        stream << (double)v;

    return stream;
}

QDataStream &operator>>( QDataStream& stream, MCMCSplineComposante& splineComposante )
{
    quint32 siz;
    double v;
    stream >> siz;

    splineComposante.vecThetaReduced.resize(siz);
    std::generate_n(splineComposante.vecThetaReduced.begin(), siz, [&stream, &v]{stream >> v; return v;});

    stream >> siz;
    splineComposante.vecG.resize(siz);
    std::generate_n(splineComposante.vecG.begin(), siz, [&stream, &v]{stream >> v; return v;});

    stream >> siz;
    splineComposante.vecGamma.resize(siz);
    std::generate_n(splineComposante.vecGamma.begin(), siz, [&stream, &v]{stream >> v; return v;});

    stream >> siz;
    splineComposante.vecVarG.resize(siz);
    std::generate_n(splineComposante.vecVarG.begin(), siz, [&stream, &v]{stream >> v; return v;});

    return stream;
};

QDataStream &operator<<( QDataStream& stream, const MCMCSpline& spline )
{
    stream << spline.splineX;
    stream << spline.splineY;
    stream << spline.splineZ;

    return stream;
}

QDataStream &operator>>( QDataStream& stream, MCMCSpline& spline )
{
    stream >> spline.splineX;
    stream >> spline.splineY;
    stream >> spline.splineZ;

    return stream;
}

QDataStream &operator<<( QDataStream& stream, const PosteriorMeanGComposante& pMGComposante )
{
    stream << (quint32) pMGComposante.vecG.size();
    for (auto& v : pMGComposante.vecG)
        stream << (double) v;

    stream << (quint32) pMGComposante.vecGP.size();
    for (auto& v : pMGComposante.vecGP)
        stream << (double)v;

/*    stream << (quint32) pMGComposante.vecVarGP.size();
    for (auto& v : pMGComposante.vecVarGP)
        stream << (double)v;
*/
    stream << (quint32) pMGComposante.vecGS.size();
    for (auto& v : pMGComposante.vecGS)
        stream << (double)v;

    stream << (quint32) pMGComposante.vecVarG.size();
    for (auto& v : pMGComposante.vecVarG)
        stream << (double)v;

    stream << pMGComposante.mapG;
    return stream;
}

QDataStream &operator>>( QDataStream& stream, PosteriorMeanGComposante& pMGComposante )
{
    quint32 siz;
    double v;

    stream >> siz;
    pMGComposante.vecG.resize(siz);
    std::generate_n(pMGComposante.vecG.begin(), siz, [&stream, &v]{stream >> v; return v;});

    stream >> siz;
    pMGComposante.vecGP.resize(siz);
    std::generate_n(pMGComposante.vecGP.begin(), siz, [&stream, &v]{stream >> v; return v;});

/*    stream >> siz;
    pMGComposante.vecVarGP.resize(siz);
    std::generate_n(pMGComposante.vecVarGP.begin(), siz, [&stream, &v]{stream >> v; return v;});
*/
    stream >> siz;
    pMGComposante.vecGS.resize(siz);
    std::generate_n(pMGComposante.vecGS.begin(), siz, [&stream, &v]{stream >> v; return v;});

    stream >> siz;
    pMGComposante.vecVarG.resize(siz);
    std::generate_n(pMGComposante.vecVarG.begin(), siz, [&stream, &v]{stream >> v; return v;});

    stream >> pMGComposante.mapG;
    return stream;
}

QDataStream &operator<<( QDataStream &stream, const PosteriorMeanG& pMeanG )
{
    stream << pMeanG.gx;
    stream << pMeanG.gy;
    stream << pMeanG.gz;

    return stream;
}

QDataStream &operator>>( QDataStream &stream, PosteriorMeanG& pMeanG )
{
    stream >> pMeanG.gx;
    stream >> pMeanG.gy;
    stream >> pMeanG.gz;

    return stream;
}


#pragma mark Calcul Spline on Event


/**
 * @brief MCMCLoopCurve::prepareCalculSpline_WI
 * With W = identity
 * @param vecH
 * @return
 */


SplineMatrices prepareCalculSpline_WI(const std::vector<t_reduceTime> &vecH)
{
    const Matrix2D& matR = calculMatR(vecH);
    const Matrix2D& matQ = calculMatQ(vecH);

    // Calcul de la transposée QT de la matrice Q, de dimension (n-2) x n

    const Matrix2D& matQT = transpose(matQ, 3);

    // Calcul de la matrice matQTW_1Q, de dimension (n-2) x (n-2) pour calcul Mat_B
    // matQTW_1Q possèdera 3+3-1=5 bandes
    MatrixDiag diagWInv (vecH.size()+1, 1);

    // Matrix2D matQTW_1Qb = multiMatParMat(matQT, matQ, 3, 3);
    const Matrix2D& matQTW_1Q = multiplyMatrixBanded_Winograd(matQT, matQ, 1);

    // Calcul de la matrice QTQ, de dimension (n-2) x (n-2) pour calcul Mat_B
    // Mat_QTQ possèdera 3+3-1=5 bandes
    // Matrix2D matQTQb = multiMatParMat(matQT, matQ, 3, 3);
    Matrix2D matQTQ = matQTW_1Q;// multiplyMatrixBanded_Winograd(matQT, matQ, 1); // pHd :: ?? on fait deux fois le même calcul !!

    SplineMatrices matrices;
    matrices.diagWInv = std::move(diagWInv);
    matrices.matR = std::move(matR);
    matrices.matQ = std::move(matQ);
    matrices.matQT = std::move(matQT);
    matrices.matQTW_1Q = std::move(matQTW_1Q); // Seule affectée par changement de VG
    matrices.matQTQ = std::move(matQTQ);

    return matrices;
}


/**
 * @brief MCMCLoopCurve::prepareCalculSpline
 * produit
 * SplineMatrices matrices;
 *  matrices.diagWInv
 *  matrices.matR
 *  matrices.matQ
 *  matrices.matQT
 *  matrices.matQTW_1Q  // Seule affectée par changement de VG
 *  matrices.matQTQ
 *
 * @param sortedEvents
 * @return
 */
SplineMatrices prepareCalculSpline(const QList<Event *> &sortedEvents, const std::vector<t_reduceTime> &vecH)
{
    const Matrix2D &rMatR = calculMatR(vecH);
    const Matrix2D &rMatQ = calculMatQ(vecH);

    // Calcul de la transposée QT de la matrice Q, de dimension (n-2) x n
    const Matrix2D &rMatQT = transpose(rMatQ, 3);

    MatrixDiag diagWInv (sortedEvents.size());
    std::transform(sortedEvents.begin(), sortedEvents.end(), diagWInv.begin(), [](Event* ev) {return 1/ev->mW;});

    // Calcul de la matrice matQTW_1Q, de dimension (n-2) x (n-2) pour calcul Mat_B
    // matQTW_1Q possèdera 3+3-1=5 bandes
    const Matrix2D &tmp = multiMatParDiag(rMatQT, diagWInv, 3);

    //Matrix2D matQTW_1Qb = multiMatParMat(tmp, matQ, 3, 3);
    const Matrix2D &rMatQTW_1Q = multiplyMatrixBanded_Winograd(tmp, rMatQ, 1);

    // Calcul de la matrice QTQ, de dimension (n-2) x (n-2) pour calcul Mat_B
    // Mat_QTQ possèdera 3+3-1=5 bandes
    //Matrix2D matQTQb = multiMatParMat(matQT, matQ, 3, 3);
    const Matrix2D &rMatQTQ = multiplyMatrixBanded_Winograd(rMatQT, rMatQ, 1);

    SplineMatrices matrices;
    matrices.diagWInv = std::move(diagWInv);
    matrices.matR = std::move(rMatR);
    matrices.matQ = std::move(rMatQ);
    matrices.matQT = std::move(rMatQT);
    matrices.matQTW_1Q = std::move(rMatQTW_1Q); // Seule affectée par changement de VG
    matrices.matQTQ = std::move(rMatQTQ);

    return matrices;
}


SplineResults do_spline(const std::function <double (Event*)> &fun, const SplineMatrices &matrices, const QList<Event *> &events, const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp, const double lambdaSpline)
{
    /*
    * MatB doit rester en copie
    */
    SplineResults spline;
    try {
        // calcul de: R + alpha * Qt * W-1 * Q = Mat_B


        // Calcul des vecteurs G et Gamma en fonction de Y
        const size_t n = events.size();

        std::vector<double> vecG;
        std::vector<double> vecQtY;

        // VecQtY doit être de taille n, donc il faut mettre un zéro au début et à la fin
        vecQtY.push_back(0.);
        for (size_t i = 1; i < n-1; ++i) {
            const double term1 = (fun(events[i+1]) - fun(events[i])) / vecH[i];
            const double term2 = (fun(events[i]) - fun(events[i-1])) / vecH[i-1];
            vecQtY.push_back(term1 - term2);
        }
        vecQtY.push_back(0.);

        // Calcul du vecteur Gamma
        const decltype(SplineResults::vecGamma) &vecGamma = resolutionSystemeLineaireCholesky(decomp, vecQtY);//, 5, 1);

        // Calcul du vecteur g = Y - lamnbda * W-1 * Q * gamma
        if (lambdaSpline != 0) {
            const std::vector<double> &vecTmp2 = multiMatParVec(matrices.matQ, vecGamma, 3);
            const MatrixDiag &diagWInv = matrices.diagWInv;

            for (unsigned i = 0; i < n; ++i) {
                vecG.push_back( fun(events[i]) - lambdaSpline * diagWInv[i] * vecTmp2[i]) ;
            }

        } else {
            vecG.resize(n);
            std::transform(events.begin(), events.end(), vecG.begin(), fun);
        }


        spline.vecG = std::move(vecG);
        spline.vecGamma = std::move(vecGamma);

    } catch(...) {
        qCritical() << "[MCMCLoopCurve::doSpline] : Caught Exception!\n";
    }

    return spline;
}


SplineResults doSplineX(const SplineMatrices &matrices, const QList<Event *> &events, const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp, const double lambdaSpline)
{return do_spline(get_Yx, matrices, events, vecH, decomp, lambdaSpline);}

SplineResults doSplineY(const SplineMatrices &matrices, const QList<Event *> &events, const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag> &decomp, const double lambdaSpline)
{return do_spline(get_Yy, matrices, events, vecH, decomp, lambdaSpline);}

SplineResults doSplineZ(const SplineMatrices &matrices, const QList<Event *> &events, const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp, const double lambdaSpline)
{return do_spline(get_Yz, matrices, events, vecH, decomp, lambdaSpline);}

/**
 Cette procedure calcule la matrice inverse de B:
 B = R + lambda * Qt * W-1 * Q
 puis calcule la matrice d'influence A(lambda)
 Bande : nombre de diagonales non nulles
 used only with MCMCLoopCurve::calcul_spline_variance()
*/
std::vector<double> calculMatInfluence_origin(const SplineMatrices& matrices, const int nbBandes, const std::pair<Matrix2D, MatrixDiag> &decomp, const double lambdaSpline)
{
    // Q_ASSERT_X(lambdaSpline!=0, "[MCMCLoopCurve::ln_h_YWI_3_update]", "lambdaSpline=0");
    /*
     * if lambda spline = 0
     * return matA.resize(n, 1.);
     */
    const size_t n = matrices.diagWInv.size();

    const Matrix2D &matB_1 = inverseMatSym_origin(decomp, nbBandes + 4, 1);

    std::vector<double> matQB_1QT;

    /*Calcul des termes diagonaux de Q (matB_1) Qt, de i=1 à nb_noeuds
             ??? vérifier le contrôle des indices i+1: débordement de matrices */


    const auto* matQi = begin(matrices.matQ[0]); // matQ est une matrice encadrée de 0
    t_matrix term = pow(matQi[0], 2) * matB_1[0][0]; // matQ[0][0] ici vaut toujours 0
    term += pow(matQi[1], 2) * matB_1[1][1];
    term += 2 * matQi[0] * matQi[1] * matB_1[0][1]; // ici vaut toujours 0
    //matQB_1QT[0] = term;
    matQB_1QT.push_back(term);

    for (size_t i = 1; i < n-1; ++i) {
        matQi = begin(matrices.matQ[i]);
        t_matrix term_i = pow(matQi[i-1], 2.) * matB_1[i-1][i-1];
        term_i += pow(matQi[i], 2.) * matB_1[i][i];
        term_i += pow(matQi[i+1], 2.) * matB_1[i+1][i+1];

        term_i += 2 * matQi[i-1] * matQi[i] * matB_1[i-1][i];
        term_i += 2 * matQi[i-1] * matQi[i+1] * matB_1[i-1][i+1];
        term_i += 2 * matQi[i] * matQi[i+1] * matB_1[i][i+1];
        //matQB_1QT[i] = term;
        matQB_1QT.push_back(term_i);
    }


    matQi = begin(matrices.matQ[n-1]);
    term = pow(matQi[n-2], 2.) * matB_1[n-2][n-2];
    term += pow(matQi[n-1], 2.) * matB_1[n-1][n-1]; // matQ[n-1][n-1] ici vaut toujours 0
    term += 2 * matQi[n-2] * matQi[n-1] * matB_1[n-2][n-1]; // matQ[n-1][n-1] ici vaut toujours 0
    //matQB_1QT[n-1] = term;
    matQB_1QT.push_back(term);


    // Multi_diag_par_Mat(Diag_W_1c, Mat_QB_1QT, Nb_noeudsc, 1, tmp1); // donne une matrice, donc la diagonale est le produit des diagonales
    // Multi_const_par_Mat(-alphac, tmp1, Nb_noeudsc,1, Mat_Ac);
    // Addit_I_et_Mat(Mat_Ac,Nb_noeudsc);
    // remplacé par:

    std::vector<double> matA;
    for (size_t i = 0; i < n; ++i) {
        matA.push_back(1 - lambdaSpline * matrices.diagWInv[i] * matQB_1QT[i]);
#if DEBUG
        if (matA[i] == 0.) {
            qWarning ("[MCMCLoopCurve::calculMatInfluence_origin] -> Oups matA.at(i) == 0 ");
        }

        if (matA[i] < 0.) {
            qDebug ()<<"[MCMCLoopCurve::calculMatInfluence_origin] -> Oups matA.at(i) ="<< matA[i] << " < 0  change to 1E-10 LambdaSpline="<<lambdaSpline;
            matA[i] = 1.E-10;
        }

        if (matA[i] > 1) {
            qWarning ("[MCMCLoopCurve::calculMatInfluence_origin] -> Oups matA.at(i) > 1  change to 1");
            matA[i] = 1.;
        }
#endif
    }

    return matA;
}


std::vector<double> calcul_spline_variance(const SplineMatrices& matrices, const QList<Event*> &events, const std::pair<Matrix2D, MatrixDiag> &decomp, const double lambdaSpline)
{
    unsigned int n = events.size();
    std::vector<double> matA = calculMatInfluence_origin(matrices, 1, decomp, lambdaSpline);
    std::vector<double> varG;

    for (unsigned int i = 0; i < n; ++i) {
#ifdef DEBUG
        const double& aii = matA[i];
        // si Aii négatif ou nul, cela veut dire que la variance sur le point est anormalement trop grande,
        // d'où une imprécision dans les calculs de Mat_B (Cf. calcul spline) et de mat_A
        if (aii <= 0.) {
            qDebug()<<"[MCMCLoopCurve] calcul_spline_variance() : Oups aii="<< aii <<"<= 0 change to 0" << "mW="<<events[i]->mW;
            varG.push_back(0.);

        } else {
            varG.push_back(matA[i]  / events[i]->mW);
        }
#else
        varG.push_back(matA[i]  / events[i]->mW);
#endif

    }

    return varG;
}


/**
 * @brief MCMCLoopCurve::currentSpline
 * @param events  update Gx , Gy and Gz of Event
 * @param doSortAndSpreadTheta
 * @param vecH
 * @param matrices
 * @return
 */
MCMCSpline currentSpline (QList<Event *> &events, const std::vector<t_reduceTime> &vecH, const SplineMatrices &matrices, const double lambda, bool doY, bool doZ)
{

    const std::vector<double> &vec_theta_red = get_vector(get_ThetaReduced, events);

    // doSpline utilise les Y des events
    // => On le calcule ici pour la première composante (x)

    Matrix2D matB; //matR;
    //const double lambda = mModel->mLambdaSpline.mX;
    if (lambda != 0) {
        const Matrix2D &tmp = multiConstParMat(matrices.matQTW_1Q, lambda, 5);
        matB = addMatEtMat(matrices.matR, tmp, 5);

    } else {
        matB = matrices.matR;
    }

    // Decomposition_Cholesky de matB en matL et matD
    // Si alpha global: calcul de Mat_B = R + alpha * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
    const std::pair<Matrix2D, MatrixDiag> &decomp = decompositionCholesky(matB, 5, 1);


    // le calcul de l'erreur est influencé par VG qui induit 1/mW, utilisé pour fabriquer matrices->DiagWinv et calculer matrices->matQTW_1Q
    // Tout le calcul précédent ne change pas

    const SplineResults &sx = doSplineX(matrices, events, vecH, decomp, lambda); // Voir si matB est utile ???
    const std::vector<double> &vecVarG = calcul_spline_variance(matrices, events, decomp, lambda); // Les erreurs sont égales sur les trois composantes X, Y, Z splineY.vecErrG = splineX.vecErrG =

    // --------------------------------------------------------------
    //  Calcul de la spline g, g" pour chaque composante x y z + stockage
    // --------------------------------------------------------------

    MCMCSplineComposante splineX;
    splineX.vecThetaReduced = vec_theta_red;
    splineX.vecG = std::move(sx.vecG);
    splineX.vecGamma = std::move(sx.vecGamma);

    splineX.vecVarG = vecVarG;

    for (int i = 0; i < events.size(); i++) {
        events[i]->mGx = splineX.vecG[i];
    }

    MCMCSpline spline;
    spline.splineX = std::move(splineX);


    if ( doY) {

        // doSpline utilise les Y des events
        // => On le calcule ici pour la seconde composante (y)

        const SplineResults &sy = doSplineY(matrices, events, vecH, decomp, lambda); //matL et matB ne sont pas changés

        MCMCSplineComposante splineY;

        splineY.vecG = std::move(sy.vecG);
        splineY.vecGamma = std::move(sy.vecGamma);
        for (int i = 0; i < events.size(); i++) {
            events[i]->mGy = splineY.vecG[i];
        }
        splineY.vecThetaReduced = vec_theta_red;
        splineY.vecVarG = vecVarG;  // Les erreurs sont égales sur les trois composantes X, Y, Z splineY.vecErrG = splineX.vecErrG =

        spline.splineY = std::move(splineY);
    }

    if ( doZ) {
        // dans le future, ne sera pas utile pour le mode sphérique
        // doSpline utilise les Z des events
        // => On le calcule ici pour la troisième composante (z)

        const SplineResults &sz = doSplineZ(matrices, events, vecH, decomp, lambda);

        MCMCSplineComposante splineZ;

        splineZ.vecG = std::move(sz.vecG);
        splineZ.vecGamma = std::move(sz.vecGamma);
        for (int i = 0; i < events.size(); i++) {
            events[i]->mGz = splineZ.vecG[i];
        }
        splineZ.vecThetaReduced= vec_theta_red;
        splineZ.vecVarG = vecVarG;

        spline.splineZ = std::move(splineZ);
    }

    return spline;
}

/**
 * @brief MCMCLoopCurve::currentSpline_WI. Lambda = 0
 * @param events must be ordered and spred
 * @param doSortAndSpreadTheta
 * @param vecH
 * @param matrices
 * @return
 */
//MCMCSpline currentSpline_WI (QList<Event *> &events, bool doSortAndSpreadTheta, const std::vector<t_reduceTime> &vecH, const SplineMatrices &matrices, bool doY, bool doZ, bool use_error)
MCMCSpline currentSpline_WI (QList<Event *> &events, bool doY, bool doZ, bool use_error)
{
    //Q_ASSERT_X(mModel->mLambdaSpline.mX!=0, "[MCMCLoopCurve::ln_h_YWI_3_update]", "lambdaSpline=0");

    const std::vector<t_reduceTime> &vecH = calculVecH(events);
    const SplineMatrices &spline_matrices = prepareCalculSpline_WI(vecH);


    const std::vector<double> &vec_theta_red = get_vector(get_ThetaReduced, events);

    // doSpline utilise les Y des events
    // => On le calcule ici pour la première composante (x)

    const Matrix2D &matB  = spline_matrices.matR;

    // Decomposition_Cholesky de matB en matL et matD
    // Si alpha global: calcul de Mat_B = R + alpha * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
    const std::pair<Matrix2D, MatrixDiag> &decomp = decompositionCholesky(matB, 5, 1);


    // le calcul de l'erreur est influencé par VG qui induit 1/mW, utilisé pour fabriquer matrices->DiagWinv et calculer matrices->matQTW_1Q
    // Tout le calcul précédent ne change pas

    std::vector<double> vecVarG;
    //if (mCurveSettings.mUseErrMesure)
    if (use_error)
        vecVarG = calcul_spline_variance(spline_matrices, events, decomp, 0.); // Les erreurs sont égales sur les trois composantes X, Y, Z splineY.vecErrG = splineX.vecErrG =
    else
        vecVarG = std::vector(events.size(), 0.);
    // --------------------------------------------------------------
    //  Calcul de la spline g, g" pour chaque composante x y z + stockage
    // --------------------------------------------------------------

    const SplineResults &sx = doSplineX(spline_matrices, events, vecH, decomp, 0.);

    MCMCSplineComposante splineX;
    splineX.vecThetaReduced = vec_theta_red;
    splineX.vecG = std::move(sx.vecG);
    splineX.vecGamma = std::move(sx.vecGamma);

    splineX.vecVarG = vecVarG;

    MCMCSpline spline;
    spline.splineX = std::move(splineX);


    if ( doY) {

        const SplineResults &sy = doSplineY(spline_matrices, events, vecH, decomp, 0.); //matL et matB ne sont pas changés

        MCMCSplineComposante splineY;

        splineY.vecG = std::move(sy.vecG);
        splineY.vecGamma = std::move(sy.vecGamma);

        splineY.vecThetaReduced = vec_theta_red;
        splineY.vecVarG = vecVarG;  // Les erreurs sont égales sur les trois composantes X, Y, Z splineY.vecErrG = splineX.vecErrG =

        spline.splineY = std::move(splineY);
    }

    if ( doZ) {
        // dans le future, ne sera pas utile pour le mode sphérique
        // doSpline utilise les Z des events
        // => On le calcule ici pour la troisième composante (z)

        const SplineResults &sz = doSplineZ(spline_matrices, events, vecH, decomp, 0.);

        MCMCSplineComposante splineZ;

        splineZ.vecG = std::move(sz.vecG);
        splineZ.vecGamma = std::move(sz.vecGamma);

        splineZ.vecThetaReduced = vec_theta_red;
        splineZ.vecVarG = vecVarG;

        spline.splineZ = std::move(splineZ);
    }

    return spline;
}


double valeurG(const double t, const MCMCSplineComposante& spline, unsigned long &i0, Model &model)
{
    const auto n = spline.vecThetaReduced.size();
    const auto tReduce = model.reduceTime(t);
    const auto t1 = spline.vecThetaReduced.at(0);
    const auto tn = spline.vecThetaReduced.at(n-1);
    double g = 0;

    if (tReduce < t1) {
        const auto t2 = spline.vecThetaReduced.at(1);
        double gp1 = (spline.vecG.at(1) - spline.vecG.at(0)) / (t2 - t1);
        gp1 -= (t2 - t1) * spline.vecGamma.at(1) / 6.;
        return (double) (spline.vecG.at(0) - (t1 - tReduce) * gp1);

    } else if (tReduce >= tn) {
        const auto tn1 = spline.vecThetaReduced.at(n-2);
        auto gpn = (spline.vecG.at(n-1) - spline.vecG.at(n-2)) / (tn - tn1);
        gpn += (tn - tn1) * spline.vecGamma.at(n-2) / 6.;
        return (double) (spline.vecG.at(n-1) + (tReduce - tn) * gpn);

    } else {
        for (; i0 < n-1; ++i0) {
            const auto ti1 = spline.vecThetaReduced.at(i0);
            const auto ti2 = spline.vecThetaReduced.at(i0+1);
            if ((tReduce >= ti1) && (tReduce < ti2)) {
                const auto h = ti2 - ti1;
                const double gi1 = spline.vecG.at(i0);
                const double gi2 = spline.vecG.at(i0+1);

                // Linear part :
                g = gi1 + (gi2-gi1)*(tReduce-ti1)/h;

                // Smoothing part :
                const auto gamma1 = spline.vecGamma.at(i0);
                const auto gamma2 = spline.vecGamma.at(i0+1);
                const auto p = (1./6.) * ((tReduce-ti1) * (ti2-tReduce)) * ((1.+(tReduce-ti1)/h) * gamma2 + (1.+(ti2-tReduce)/h) * gamma1);
                g -= p;

                break;
            }
        }
    }

    return g;
}

// ------------------------------------------------------------------
//  Valeur de Err_G en t à partir de Vec_Err_G
//  par interpolation linéaire des erreurs entre les noeuds
// ------------------------------------------------------------------
//obsolete
double valeurErrG(const double t, const MCMCSplineComposante& spline, unsigned& i0, Model &model)
{
    const unsigned n = spline.vecThetaReduced.size();
    const double t_red = model.reduceTime(t);

    const double t1 = spline.vecThetaReduced[0];
    const double tn = spline.vecThetaReduced[n-1];
    double errG = 0;

    if (t_red < t1) {
        errG = sqrt(spline.vecVarG.at(0));

    } else if (t_red >= tn) {
        errG = sqrt(spline.vecVarG.at(n-1));

    } else {
        for (; i0 <n-1; ++i0) {
            const auto ti1 = spline.vecThetaReduced[i0];
            const auto ti2 = spline.vecThetaReduced[i0+1];
            if ((t_red >= ti1) && (t < ti2)) {

                const double err1 = sqrt(spline.vecVarG[i0]);
                const double err2 = sqrt(spline.vecVarG[i0+1]);
                errG = err1 + ((t_red-ti1) / (ti2-ti1)) * (err2 - err1);
                break;
            }
        }
    }

    return errG;
}

// dans RenCurve U-CMT-Routine_Spline Valeur_Gp
double valeurGPrime(const double t, const MCMCSplineComposante& spline, unsigned& i0, Model &model)
{
    const unsigned n = spline.vecThetaReduced.size();
    const double tReduce =  model.reduceTime(t);
    const double t1 = spline.vecThetaReduced.at(0);
    const double tn = spline.vecThetaReduced.at(n-1);
    double gPrime = 0.;

    // la dérivée première est toujours constante en dehors de l'intervalle [t1,tn]
    if (tReduce < t1) {
        const double t2 = spline.vecThetaReduced.at(1);
        gPrime = (spline.vecG.at(1) - spline.vecG.at(0)) / (t2 - t1);
        gPrime -= (t2 - t1) * spline.vecGamma.at(1) / 6.;


    } else if (tReduce >= tn) {
        const double tin_1 = spline.vecThetaReduced.at(n-2);
        gPrime = (spline.vecG.at(n-1) - spline.vecG.at(n-2)) / (tn - tin_1);
        gPrime += (tn - tin_1) * spline.vecGamma.at(n-2) / 6.;


    } else {
        for ( ;i0< n-1; ++i0) {
            const auto ti1 = spline.vecThetaReduced.at(i0);
            const auto ti2 = spline.vecThetaReduced.at(i0+1);
            if ((tReduce >= ti1) && (tReduce < ti2)) {
                const auto h = ti2 - ti1;
                const double gi1 = spline.vecG.at(i0);
                const double gi2 = spline.vecG.at(i0+1);
                const double gamma1 = spline.vecGamma.at(i0);
                const double gamma2 = spline.vecGamma.at(i0+1);

                gPrime = ((gi2-gi1)/h) - (1./6.) * (tReduce-ti1) * (ti2-tReduce) * ((gamma2-gamma1)/h);
                gPrime += (1./6.) * ((tReduce-ti1) - (ti2-tReduce)) * ( (1.+(tReduce-ti1)/h) * gamma2 + (1+(ti2-tReduce)/h) * gamma1 );

                break;
            }
        }

    }

    return gPrime;
}

double valeurGSeconde(const double t, const MCMCSplineComposante& spline, Model &model)
{
    const int n = spline.vecThetaReduced.size();
    const auto tReduce = model.reduceTime(t);
    // The second derivative is always zero outside the interval [t1,tn].
    double gSeconde = 0.;

    for (int i = 0; i < n-1; ++i) {
        const t_reduceTime ti1 = spline.vecThetaReduced.at(i);
        const t_reduceTime ti2 = spline.vecThetaReduced.at(i+1);
        if ((tReduce >= ti1) && (tReduce < ti2)) {
            const t_reduceTime h = ti2 - ti1;
            const double gamma1 = spline.vecGamma.at(i);
            const double gamma2 = spline.vecGamma.at(i+1);
            gSeconde = ((tReduce-ti1) * gamma2 + (ti2-tReduce) * gamma1) / h;
            break;
        }
    }

    return gSeconde;
}


void valeurs_G_VarG_GP_GS(const double t, const MCMCSplineComposante &spline, double& G, double& varG, double& GP, double& GS, unsigned& i0, double tmin, double tmax)
{
    const unsigned long n = spline.vecThetaReduced.size();
    const t_reduceTime tReduce =  (t - tmin)/(tmax - tmin);
    const t_reduceTime t1 = spline.vecThetaReduced.at(0);
    const t_reduceTime tn = spline.vecThetaReduced.at(n-1);
    GP = 0.;
    GS = 0.;
    double h;

    // The first derivative is always constant outside the interval [t1,tn].
    if (tReduce < t1) {
        const t_reduceTime t2 = spline.vecThetaReduced.at(1);

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

        const t_reduceTime tn1 = spline.vecThetaReduced.at(n-2);

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
            const t_reduceTime ti1 = spline.vecThetaReduced.at(i0);
            const t_reduceTime ti2 = spline.vecThetaReduced.at(i0 + 1);
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
    GP /= (tmax - tmin);
    GS /= pow(tmax - tmin, 2.);
}

#pragma mark Calcul Spline on vector Y
// old prepareCalculSpline()
SplineMatrices prepare_calcul_spline(const std::vector<t_reduceTime> &vecH, const std::vector<double> vec_Vg_Si2)
{
    const Matrix2D &matR = calculMatR(vecH);
    const Matrix2D &matQ = calculMatQ(vecH);

    // Calcul de la transposée QT de la matrice Q, de dimension (n-2) x n
    const Matrix2D &matQT = transpose(matQ, 3);

    MatrixDiag diagWInv  (vec_Vg_Si2.size());
    std::transform(vec_Vg_Si2.begin(), vec_Vg_Si2.end(), diagWInv.begin(), [](double v) {return v;});

    // Calcul de la matrice matQTW_1Q, de dimension (n-2) x (n-2) pour calcul Mat_B
    // matQTW_1Q possèdera 3+3-1=5 bandes
    const Matrix2D &tmp = multiMatParDiag(matQT, diagWInv, 3);

    //Matrix2D matQTW_1Qb = multiMatParMat(tmp, matQ, 3, 3);
    const Matrix2D &matQTW_1Q = multiplyMatrixBanded_Winograd(tmp, matQ, 1);

    // Calcul de la matrice QTQ, de dimension (n-2) x (n-2) pour calcul Mat_B
    // Mat_QTQ possèdera 3+3-1=5 bandes
    const Matrix2D &matQTQ = multiplyMatrixBanded_Winograd(matQT, matQ, 1);

    SplineMatrices matrices;
    matrices.diagWInv = std::move(diagWInv);
    matrices.matR = std::move(matR);
    matrices.matQ = std::move(matQ);
    matrices.matQT = std::move(matQT);
    matrices.matQTW_1Q = std::move(matQTW_1Q); // Seule affectée par changement de VG
    matrices.matQTQ = std::move(matQTQ);

    return matrices;
}

std::vector<double> calcul_spline_variance(const SplineMatrices& matrices, const std::pair<Matrix2D, MatrixDiag> &decomp, const double lambdaSpline)
{

    const std::vector<double> &matA = calculMatInfluence_origin(matrices, 1, decomp, lambdaSpline);
    std::vector<double> varG;
    const auto &diagW1 = matrices.diagWInv;
    unsigned int n = diagW1.size();

    for (unsigned int i = 0; i < n; ++i) {
#ifdef DEBUG
        const double& aii = matA[i];
        // si Aii négatif ou nul, cela veut dire que la variance sur le point est anormalement trop grande,
        // d'où une imprécision dans les calculs de Mat_B (Cf. calcul spline) et de mat_A
        if (aii <= 0.) {
            qDebug()<<"[MCMCLoopCurve] calcul_spline_variance() : Oups aii="<< aii <<"<= 0 change to 0" << "1/mW="<<(double)diagW1[i];
            varG.push_back(0.);

        } else {
            varG.push_back(matA[i]  * diagW1[i]);
        }
#else
        varG.push_back(matA[i]  * diagW1[i]);
#endif

    }

    return varG;
}

std::vector<QMap<double, double>> composante_to_curve(MCMCSplineComposante spline_compo, double tmin, double tmax, double step)
{
    QMap<double, double> curve;
    QMap<double, double> curve_plus;
    QMap<double, double> curve_moins;
    double g, varG, gp, gs;
    int nb_pts = (tmax-tmin)/step + 1 +1;
    unsigned i0 = 0;
    for (int i= 0; i < nb_pts ; ++i) {
        const double t = (double)i * step + tmin ;
        valeurs_G_VarG_GP_GS(t, spline_compo, g, varG, gp, gs, i0, tmin, tmax);
        curve[t] = g;
        curve_plus[t] = g + 1.96*sqrt(varG);
        curve_moins[t] = g - 1.96*sqrt(varG);
    }
    return std::vector<QMap<double, double>>({curve, curve_plus, curve_moins});
}


/**
 * @brief MCMCLoopCurve::doSpline
 * fabrication de spline avec
 *      spline.vecG
 *      spline.vecGamma
 *
 * @param matrices
 * @param events
 * @param vecH
 * @param decomp
 * @param lambdaSpline
 *
 * @return SplineResults
 */
SplineResults do_spline(const std::vector<double> &vec_Y, const SplineMatrices &matrices, const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp, const double lambdaSpline)
{
    /*
    * MatB doit rester en copie
    */
    SplineResults spline;
    try {
        // calcul de: R + alpha * Qt * W-1 * Q = Mat_B

        // Calcul des vecteurs G et Gamma en fonction de Y
        const size_t n = vec_Y.size();

        std::vector<double> vecG;
        std::vector<double> vecQtY;

        // VecQtY doit être de taille n, donc il faut mettre un zéro au début et à la fin
        vecQtY.push_back(0.);
        for (size_t i = 1; i < n-1; ++i) {
            const double term1 = (vec_Y[i+1] - vec_Y[i]) / vecH[i];
            const double term2 = (vec_Y[i] - vec_Y[i-1]) / vecH[i-1];
            vecQtY.push_back(term1 - term2);
        }
        vecQtY.push_back(0.);

        // Calcul du vecteur Gamma
        const decltype(SplineResults::vecGamma) &vecGamma = resolutionSystemeLineaireCholesky(decomp, vecQtY);
        // Calcul du vecteur g = Y - lamnbda * W-1 * Q * gamma
        if (lambdaSpline != 0) {
            const std::vector<double> &vecTmp2 = multiMatParVec(matrices.matQ, vecGamma, 3);
            const MatrixDiag &diagWInv = matrices.diagWInv;

            for (unsigned i = 0; i < n; ++i) {
                const double g = vec_Y[i] - lambdaSpline * diagWInv[i] * vecTmp2[i];

                if (std::isnan(g)) {
                    qDebug()<< " isnan(g)";
                    vecG.push_back( +INFINITY) ;
                } else {
                   vecG.push_back( g) ;
                }

            }

        } else {
            vecG = vec_Y;
        }


        spline.vecG = std::move(vecG);
        spline.vecGamma = std::move(vecGamma);

    } catch(...) {
        qCritical() << "[do_Spline] : Caught Exception!\n";
    }

    return spline;
}


MCMCSpline do_spline_composante(const QMap<double, double> &data_X, const QMap<double, double> &data_X_err, double tmin, double tmax, const CurveSettings &cs, const QMap<double, double> &data_Y, const QMap<double, double> &data_Y_err, const QMap<double, double> &data_Z, const QMap<double, double> &data_Z_err)
{
    std::vector<double> vecH;//double lambda, bool do_cross_validation)

    std::vector<double> vec_theta_red;
    std::vector<double> vec_X, vec_Y, vec_Z;
    std::vector<double> vec_X_err, vec_Y_err, vec_Z_err;

    bool doY (!data_Y.empty() && data_Z.empty());
    bool doYZ (!data_Y.empty() && !data_Z.empty());

    auto iter = data_X.begin();
    for (; iter != std::prev(data_X.end()) ; iter++) {
        vecH.push_back((std::next(iter).key() -  iter.key())/(tmax-tmin));

        vec_theta_red.push_back((iter.key()-tmin)/(tmax-tmin));
        vec_X.push_back(iter.value());

    }
    //iter++;
    vec_theta_red.push_back((iter.key()-tmin)/(tmax-tmin));
    vec_X.push_back(iter.value());

    const SplineMatrices &spline_matrices_WI = prepareCalculSpline_WI(vecH);

    // doSpline utilise les Y des events
    // => On le calcule ici pour la première composante (x)

    Matrix2D matB;

    for (auto value : data_X_err.values()) {
        vec_X_err.push_back(value);
    }

    if (doY || doYZ) {
        for (auto value : data_Y.values()) {
            vec_Y.push_back(value);
        }
        for (auto value : data_Y_err.values()) {
            vec_Y_err.push_back(value);
        }
    }

    if (doYZ) {
        for (auto value : data_Z.values()) {
            vec_Z.push_back(value);
        }
        for (auto value : data_Z_err.values()) {
            vec_Z_err.push_back(value);
        }
    }



    double lambda_cv;
    if (cs.mLambdaSplineType != CurveSettings::eModeFixed) {
        if (cs.mVarianceType == CurveSettings::eModeFixed) {
            lambda_cv = initLambdaSplineByCV_VgFixed(vec_X, vec_X_err, vecH , cs.mVarianceFixed);

        } else {
            lambda_cv = initLambdaSplineByCV(vec_X, vec_X_err, spline_matrices_WI, vecH , vec_Y, vec_Y_err, vec_Z, vec_Z_err);
        }

    } else {
        lambda_cv = cs.mLambdaSpline;
    }

    qDebug()<<" end of cross_validation lambda : "<<lambda_cv<<" = 10E"<<log10(lambda_cv);



    // le calcul de l'erreur est influencé par VG qui induit 1/mW, utilisé pour fabriquer matrices->DiagWinv et calculer matrices->matQTW_1Q
    // Tout le calcul précédent ne change pas

    //const SplineResults &sx_WI = do_spline(vec_Y, spline_matrices_WI,  vecH, decomp, lambda_cv); // Voir si matB est utile ???

    // calcul error

    double Vg = 0;
    if (cs.mVarianceType == CurveSettings::eModeFixed) {
        Vg = cs.mVarianceFixed;

    } else { // si individuel ou global VG = S02
        // S02_Vg_Yx() Utilise la valeur de lambda courant, sert à initialise S02_Vg
        Vg = var_residual(vec_X, spline_matrices_WI, vecH, lambda_cv);

        if (doY) {
            Vg += var_residual(vec_Y, spline_matrices_WI, vecH, lambda_cv);
            Vg /= 2.;
        } else if (doYZ) {
            Vg += var_residual(vec_Y, spline_matrices_WI, vecH, lambda_cv) + var_residual(vec_Z, spline_matrices_WI, vecH, lambda_cv);
            Vg /= 3.;
        }


    }

    std::vector<double> vec_Vg_Si2;
    for (auto [key, value] : data_X_err.asKeyValueRange()) {
        vec_Vg_Si2.push_back(Vg + pow(value, 2.));
    }

    SplineMatrices spline_matrices = prepare_calcul_spline(vecH, vec_Vg_Si2);
/*
// 2d test
    const double lambda_cv2 = cs.mLambdaSplineType != CurveSettings::eModeFixed ? initLambdaSplineByCV(vec_Y, vec_Y_err,spline_matrices, vecH ) : cs.mLambdaSpline;
    lambda_cv = lambda_cv2;
    qDebug()<<" cross_validation lambda2 : "<<lambda_cv2<<" = 10E"<<log10(lambda_cv);

// 3d test
    if (cs.mVarianceType == CurveSettings::eModeFixed) {
        Vg = cs.mVarianceFixed;

    } else { // si individuel ou global VG = S02
        // S02_Vg_Yx() Utilise la valeur de lambda courant, sert à initialise S02_Vg
        Vg = var_residual(vec_Y, spline_matrices, vecH, lambda_cv);
        //std::cout<<" var_residu_X = " << var_residu_X;


    }

    vec_Vg_Si2.clear();
    for (auto [key, value] : data_err.asKeyValueRange()) {
        vec_Vg_Si2.push_back(Vg + pow(value, 2.));
    }

    spline_matrices = prepare_calcul_spline(vecH, vec_Vg_Si2);
    const double lambda_cv3 = cs.mLambdaSplineType != CurveSettings::eModeFixed ? initLambdaSplineByCV(vec_Y, vec_Y_err, spline_matrices, vecH ) : cs.mLambdaSpline;
    lambda_cv = lambda_cv3;
    qDebug()<<"end of cross_validation lambda3 : "<<lambda_cv3<<" = 10E"<<log10(lambda_cv);
*/
  //  Spline final
    if (lambda_cv != 0) {
        const Matrix2D &tmp = multiConstParMat(spline_matrices.matQTW_1Q, lambda_cv, 5);
        matB = addMatEtMat(spline_matrices.matR, tmp, 5);

    } else {
        matB = spline_matrices.matR;
    }
    // Decomposition_Cholesky de matB en matL et matD
    // Si alpha global: calcul de Mat_B = R + alpha * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
    const std::pair<Matrix2D, MatrixDiag> &decomp = decompositionCholesky(matB, 5, 1);

    const SplineResults &sx = do_spline(vec_X, spline_matrices,  vecH, decomp, lambda_cv);

    const std::vector<double> &vecVarG = calcul_spline_variance(spline_matrices, decomp, lambda_cv);

    // --------------------------------------------------------------
    //  Calcul de la spline g, g" pour chaque composante x y z + stockage
    // --------------------------------------------------------------
    MCMCSpline spline;

    MCMCSplineComposante splineX, splineY, splineZ;

    splineX.vecThetaReduced = vec_theta_red;
    splineX.vecG = std::move(sx.vecG);
    splineX.vecGamma = std::move(sx.vecGamma);

    splineX.vecVarG = vecVarG;


    if (doY || doYZ) {
        const SplineResults &sy = do_spline(vec_Y, spline_matrices,  vecH, decomp, lambda_cv);

        splineY.vecThetaReduced = vec_theta_red;
        splineY.vecG = std::move(sy.vecG);
        splineY.vecGamma = std::move(sy.vecGamma);

        splineY.vecVarG = vecVarG;

        if (doYZ) {
            const SplineResults &sz = do_spline(vec_Z, spline_matrices,  vecH, decomp, lambda_cv);

            splineZ.vecThetaReduced = vec_theta_red;
            splineZ.vecG = std::move(sz.vecG);
            splineZ.vecGamma = std::move(sz.vecGamma);

            splineZ.vecVarG = vecVarG;
        }
    }

    spline.splineX = std::move(splineX);
    spline.splineY = std::move(splineY);
    spline.splineZ = std::move(splineZ);

    return spline;
}


double cross_validation (const std::vector< double> &vec_Y, const SplineMatrices &matrices, const std::vector< double> &vecH, const double lambdaSpline)
{

    const double N = matrices.diagWInv.size();

    Matrix2D matB = addMatEtMat(matrices.matR, multiConstParMat(matrices.matQTW_1Q, lambdaSpline, 5), 5);

    // Decomposition_Cholesky de matB en matL et matD
    // Si alpha global: calcul de Mat_B = R + alpha * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
    const auto &decomp = decompositionCholesky(matB, 5, 1);

    const SplineResults &s = do_spline(vec_Y, matrices,  vecH, decomp, lambdaSpline);

    //auto matA = calculMatInfluence_origin(matrices, s , 1, lambdaSpline);
    const std::vector<double> &matA = calculMatInfluence_origin(matrices, 1, decomp, lambdaSpline);

    double CV = 0.;
    for (int i = 0 ; i < N; i++) {
        CV +=  pow((s.vecG.at(i) - vec_Y[i]) / (1-matA.at(i)), 2.) ;  // / matrices.diagWInv.at(i)=1 ;
    }

    return CV;
}

// Cross-validation initialization
double initLambdaSplineByCV(const std::vector< double> &vec_X, const std::vector< double> &vec_X_err, const SplineMatrices &matrices, const std::vector< double> &vecH,const std::vector< double> &vec_Y, const std::vector< double> &vec_Y_err, const std::vector< double> &vec_Z, const std::vector< double> &vec_Z_err)
{
    std::vector< double> GCV, CV, lambda_GCV, lambda_CV;
    bool doY = !vec_Y.empty() && vec_Z.empty();
    bool doYZ = !vec_Y.empty() && !vec_Z.empty();

    for (int idxLambda = -200; idxLambda < 101; ++idxLambda ) {
        const double lambda_loop = pow(10., ( double)idxLambda/10.);

        double Vg = var_residual(vec_X, matrices, vecH, lambda_loop);

        if (doY) {
            Vg += var_residual(vec_Y, matrices, vecH, lambda_loop);
            Vg /= 2.;

        } else if (doYZ) {
            Vg += var_residual(vec_Y, matrices, vecH, lambda_loop) + var_residual(vec_Z, matrices, vecH, lambda_loop);
            Vg /= 3.;
        }


        std::vector<double> vec_Vg_Si2;

        for (auto X_err : vec_X_err) {
            vec_Vg_Si2.push_back(Vg + pow(X_err, 2.));
        }

        const SplineMatrices &test_matrices = prepare_calcul_spline(vecH, vec_Vg_Si2);
        //
        const double cv = cross_validation(vec_X, test_matrices, vecH, lambda_loop);
        const double gcv = general_cross_validation(vec_X, test_matrices, vecH, lambda_loop);

        if (!std::isnan(gcv) && !std::isinf(gcv)) {
            GCV.push_back(gcv);
            lambda_GCV.push_back(lambda_loop);
            //qDebug()<<" gcv="<<idxLambda<<GCV.back();
        }

        if (!std::isnan(cv) && !std::isinf(cv)) {
            CV.push_back(cv);
            lambda_CV.push_back(lambda_loop);
            //qDebug()<<" cv="<<idxLambda<<CV.back();
        }
    }

    // We are looking for the smallest CV value
    unsigned long idxDifMin = std::distance(GCV.begin(), std::min_element(GCV.begin(), GCV.end()) );

    unsigned long idxDifMinCV = std::distance(CV.begin(), std::min_element(CV.begin(), CV.end()) );


    // If the mini is at one of the bounds, there is no solution in the interval for GCV
    // See if there is a solution in CV
     if (idxDifMin == 0 || idxDifMin == (GCV.size()-1)) {
        qDebug()<<" 2d chance\t idxDifMin CV ="<<idxDifMinCV<< "Pas de solution avec GCV idxDifMin GCV = 00 "<<idxDifMin;

        if (idxDifMinCV == 0 || idxDifMinCV == (CV.size()-1)) {
            qDebug()<<"[initLambdaSplineByCV] No solution with CV ="<<idxDifMinCV<< " On prend lambda_GCV[0] "<<lambda_GCV.at(0);
            return lambda_GCV.at(0);

        } else {
            qDebug()<<"[initLambdaSplineByCV] With CV lambda_CV.at("<<idxDifMinCV<<")= "<<lambda_CV.at(idxDifMinCV);
            return lambda_CV.at(idxDifMinCV);
        }

     } else {
        qDebug()<<"[initLambdaSplineByCV] With GCV lambda_GCV.at("<<idxDifMin<<")= "<<lambda_GCV.at(idxDifMin);
        return lambda_GCV.at(idxDifMin);
     }


}

double initLambdaSplineByCV_VgFixed(const std::vector< double> &vec_Y, const std::vector< double> &vec_Y_err, const std::vector< double> &vecH, const double Vg)
{
     std::vector< double> GCV, CV, lambda_GCV, lambda_CV;

     std::vector<double> vec_Vg_Si2;

     for (auto Yerr : vec_Y_err) {
        vec_Vg_Si2.push_back(Vg + pow(Yerr, 2.));
     }

     for (int idxLambda = -200; idxLambda < 101; ++idxLambda ) {
        const double lambda_loop = pow(10., ( double)idxLambda/10.);

        const SplineMatrices &test_matrices = prepare_calcul_spline(vecH, vec_Vg_Si2);
        //
        const double cv = cross_validation(vec_Y, test_matrices, vecH, lambda_loop);
        const double gcv = general_cross_validation(vec_Y, test_matrices, vecH, lambda_loop);

        if (!std::isnan(gcv) && !std::isinf(gcv)) {
            GCV.push_back(gcv);
            lambda_GCV.push_back(lambda_loop);
            //qDebug()<<" gcv="<<idxLambda<<GCV.back();
        }

        if (!std::isnan(cv) && !std::isinf(cv)) {
            CV.push_back(cv);
            lambda_CV.push_back(lambda_loop);
            //qDebug()<<" cv="<<idxLambda<<CV.back();
        }
     }

     // We are looking for the smallest CV value
     unsigned long idxDifMin = std::distance(GCV.begin(), std::min_element(GCV.begin(), GCV.end()) );

     unsigned long idxDifMinCV = std::distance(CV.begin(), std::min_element(CV.begin(), CV.end()) );


     // If the mini is at one of the bounds, there is no solution in the interval for GCV
     // See if there is a solution in CV
     if (idxDifMin == 0 || idxDifMin == (GCV.size()-1)) {
        qDebug()<<" 2d chance\t idxDifMin CV ="<<idxDifMinCV<< "Pas de solution avec GCV idxDifMin GCV = 00 "<<idxDifMin;

        if (idxDifMinCV == 0 || idxDifMinCV == (CV.size()-1)) {
            qDebug()<<"[initLambdaSplineByCV_VgFixed] No solution with CV ="<<idxDifMinCV<< " On prend lambda_GCV[0] "<<lambda_GCV.at(0);
            return lambda_GCV.at(0);

        } else {
            qDebug()<<"[initLambdaSplineByCV_VgFixed] With CV lambda_CV.at("<<idxDifMinCV<<")= "<<lambda_CV.at(idxDifMinCV);
            return lambda_CV.at(idxDifMinCV);
        }

     } else {
        qDebug()<<"[initLambdaSplineByCV_VgFixed] With GCV lambda_GCV.at("<<idxDifMin<<")= "<<lambda_GCV.at(idxDifMin);
        return lambda_GCV.at(idxDifMin);
     }


}
/**
 * @brief general_cross_validation i.e. the prediction of (g_(ti)-Yij) without the point Yij
 * @param vec_Y
 * @param matrices
 * @param vecH
 * @param lambdaSpline
 * @return
 */
double general_cross_validation (const std::vector< double>& vec_Y,  const SplineMatrices& matrices, const std::vector<double>& vecH, const double lambdaSpline)
{
    const double N = matrices.diagWInv.size();

    Matrix2D matB = addMatEtMat(matrices.matR, multiConstParMat(matrices.matQTW_1Q, lambdaSpline, 5), 5);

    // Decomposition_Cholesky de matB en matL et matD
    // Si alpha global: calcul de Mat_B = R + alpha * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
    //std::pair<Matrix2D, std::vector<double>> decomp = decompositionCholesky(matB, 5, 1);
    //SplineResults s = calculSplineX (matrices, vecH, decomp, matB, lambdaSpline);

    //auto matA = calculMatInfluence_origin(matrices, s , 1, lambdaSpline);
    const auto &decomp = decompositionCholesky(matB, 5, 1);

    const SplineResults &s = do_spline(vec_Y, matrices,  vecH, decomp, lambdaSpline);
    const std::vector<double> &matA = calculMatInfluence_origin(matrices, 1, decomp, lambdaSpline);

    // Nombre de degré de liberté
    auto DLEc = N - std::accumulate(matA.begin(), matA.end(), 0.);

    double GCV = 0.;
    for (int i = 0 ; i < N; i++) {
        // utiliser mYx pour splineX
        GCV +=  pow(s.vecG.at(i) - vec_Y[i], 2.)/ matrices.diagWInv.at(i) ;
    }
    GCV /= pow(DLEc, 2.);


    return GCV;
}

double var_residual(const std::vector<double> &vec_Y, const SplineMatrices &matrices, const std::vector<t_reduceTime> &vecH, const double lambda)
{
    Matrix2D matB;
    if (lambda != 0) {
        const Matrix2D &tmp = multiConstParMat(matrices.matQTW_1Q, lambda, 5);
        matB = addMatEtMat(matrices.matR, tmp, 5);

    } else {
        matB = matrices.matR;
    }

    // Decomposition_Cholesky de matB en matL et matD
    // Si alpha global: calcul de Mat_B = R + alpha * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
    const std::pair<Matrix2D, MatrixDiag> &decomp = decompositionCholesky(matB, 5, 1);

    const SplineResults &sy = do_spline(vec_Y, matrices,  vecH, decomp, lambda);

    const std::vector< double> &vecG = sy.vecG;

    double res = 0.;

    auto g = vecG.begin();
    for (auto& Y : vec_Y) {
        res += pow(Y - *g++, 2.);
    }

    const std::vector< double> &matA = calculMatInfluence_origin(matrices, 1, decomp, lambda);

    const double traceA = std::accumulate(matA.begin(), matA.end(), 0.);

    res /= (double)(vecG.size()) - traceA;
    return std::move(res);

}
