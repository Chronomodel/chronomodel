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

#include "CurveUtilities.h"
#include "Model.h"

#include <algorithm>
#include <iostream>

extern QString res_file_version;


std::vector<t_reduceTime> calculVecH(const std::vector<std::shared_ptr<Event> > &event)
{
    std::vector<double> result (event.size()-1);
    // Définition d'une lambda pour la différence
    auto diffX = [](const std::shared_ptr<Event>& e0, const std::shared_ptr<Event>& e1) -> t_reduceTime {
        return e1->mThetaReduced - e0->mThetaReduced;
    };

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
    const unsigned long n = rVecH.size() + 1;

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
        matR[i][i] = (rVecH[i-1] + rVecH[i]) / 3.0L;
        matR[i][i+1] = rVecH[i] / 6.0L;
        matR[i+1][i] = rVecH[i] / 6.0L;
    }
    // Si on est en n-2 (dernière itération), on ne calcule pas les valeurs de part et d'autre de la diagonale (termes symétriques)
   matR[n-2][n-2] = (rVecH[n-2-1] + rVecH[n-2]) / 3.0;

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
        matQ[i-1][i] = 1.0L  / rVecH[i-1];
        matQ[i][i] = -((1.0L / rVecH[i-1]) + (1.0L /rVecH[i]));
        matQ[i+1][i] = 1.0L  / rVecH[i];

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
        // Calcul de la boule d'erreur moyenne par moyenne quadratique ligne 464
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

    stream << (quint32) pMGComposante.vecGS.size();
    for (auto& v : pMGComposante.vecGS)
        stream << (double)v;

    stream << (quint32) pMGComposante.vecVarG.size();
    for (auto& v : pMGComposante.vecVarG)
        stream << (double)v;

    stream << pMGComposante.mapG;

    stream << pMGComposante.mapGP; // since v3.2.7
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
    if (res_file_version>"3.2.6")
        stream >> pMGComposante.mapGP;
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


SplineMatrices prepare_calcul_spline_WI(const std::vector<t_reduceTime> &vecH)
{
    const Matrix2D& matR = calculMatR(vecH);
    const Matrix2D& matQ = calculMatQ(vecH);

    // Calcul de la transposée QT de la matrice Q, de dimension (n-2) x n

    const Matrix2D& matQT = transpose(matQ, 3);

    // Calcul de la matrice matQTW_1Q, de dimension (n-2) x (n-2) pour calcul Mat_B
    // matQTW_1Q possèdera 3+3-1=5 bandes
    MatrixDiag diagWInv (vecH.size()+1, 1.0L);

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
    matrices.matQTW_1Q = std::move(matQTW_1Q);
    matrices.matQTQ = std::move(matQTQ);

    return matrices;
}


/**
 * @brief MCMCLoopCurve::prepareCalculSpline
 * produit
 * SplineMatrices matrices;
 *  matrices.diagWInv  // utilise mW
 *  matrices.matR   // utilise vecH
 *  matrices.matQ   // utilise vecH
 *  matrices.matQT
 *  matrices.matQTW_1Q  // Seule affectée par changement de VG
 *  matrices.matQTQ
 *
 * @param sortedEvents
 * @return
 */
SplineMatrices prepare_calcul_spline(const std::vector<std::shared_ptr<Event>>& sortedEvents, const std::vector<t_reduceTime>& vecH)
{
    MatrixDiag W_Inv (sortedEvents.size());
    std::transform(sortedEvents.begin(), sortedEvents.end(), W_Inv.begin(), [](std::shared_ptr<Event> ev) {return 1.0L /ev->mW;});

    return prepare_calcul_spline(vecH, W_Inv);

   /* const Matrix2D &rMatR = calculMatR(vecH);
    const Matrix2D &rMatQ = calculMatQ(vecH);

    // Calcul de la transposée QT de la matrice Q, de dimension (n-2) x n
    const Matrix2D &rMatQT = transpose(rMatQ, 3);

    MatrixDiag diagWInv (sortedEvents.size());
    std::transform(sortedEvents.begin(), sortedEvents.end(), diagWInv.begin(), [](std::shared_ptr<Event> ev) {return 1.0L /ev->mW;});

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

    return matrices;*/
}

SplineMatrices prepare_calcul_spline(const std::vector<t_reduceTime>& vecH, const std::vector<long double> W_1)
{
    const Matrix2D& matR = calculMatR(vecH);
    const Matrix2D& matQ = calculMatQ(vecH);

    // Calcul de la transposée QT de la matrice Q, de dimension (n-2) x n
    const Matrix2D& matQT = transpose(matQ, 3);

    MatrixDiag diagWInv  (W_1);
    // std::transform(W_1.begin(), W_1.end(), diagWInv.begin(), [](double v) {return v;});

    // Calcul de la matrice matQTW_1Q, de dimension (n-2) x (n-2) pour calcul Mat_B
    // matQTW_1Q possèdera 3+3-1=5 bandes
    const Matrix2D& tmp = multiMatParDiag(matQT, diagWInv, 3);

    //Matrix2D matQTW_1Qb = multiMatParMat(tmp, matQ, 3, 3);
    const Matrix2D& matQTW_1Q = multiplyMatrixBanded_Winograd(tmp, matQ, 1);

    // Calcul de la matrice QTQ, de dimension (n-2) x (n-2) pour calcul Mat_B
    // Mat_QTQ possèdera 3+3-1=5 bandes
    const Matrix2D& matQTQ = multiplyMatrixBanded_Winograd(matQT, matQ, 1);

    SplineMatrices matrices;
    matrices.diagWInv = std::move(diagWInv);
    matrices.matR = std::move(matR);
    matrices.matQ = std::move(matQ);
    matrices.matQT = std::move(matQT);
    matrices.matQTW_1Q = std::move(matQTW_1Q); // Seule affectée par changement de VG
    matrices.matQTQ = std::move(matQTQ);

    return matrices;
}
// On utilise la copie au passage du parametre spline_matrices
SplineMatrices update_splineMatrice_with_vecH(SplineMatrices spline_matrices, const std::vector<t_reduceTime>& vecH)
{
    spline_matrices.matR = calculMatR(vecH);
    spline_matrices.matQ = calculMatQ(vecH);

    // Calcul de la transposée QT de la matrice Q, de dimension (n-2) x n
    spline_matrices.matQT = transpose(spline_matrices.matQ, 3);

    //MatrixDiag diagWInv (sortedEvents.size());
    //std::transform(sortedEvents.begin(), sortedEvents.end(), diagWInv.begin(), [](std::shared_ptr<Event> ev) {return 1.0 /ev->mW;});

    // Calcul de la matrice matQTW_1Q, de dimension (n-2) x (n-2) pour calcul Mat_B
    // matQTW_1Q possèdera 3+3-1=5 bandes
    const Matrix2D &tmp = multiMatParDiag(spline_matrices.matQT, spline_matrices.diagWInv, 3);

    spline_matrices.matQTW_1Q = multiplyMatrixBanded_Winograd(tmp, spline_matrices.matQ, 1); // Seule affectée par changement de VG

    // Calcul de la matrice QTQ, de dimension (n-2) x (n-2) pour calcul Mat_B
    // Mat_QTQ possèdera 3+3-1=5 bandes
    spline_matrices.matQTQ  = multiplyMatrixBanded_Winograd(spline_matrices.matQT, spline_matrices.matQ, 1);

    return spline_matrices;
}
SplineMatrices update_splineMatrice_with_mW(SplineMatrices spline_matrices, const std::vector<std::shared_ptr<Event> >& sortedEvents)
{
    //const Matrix2D &rMatR = calculMatR(vecH);
    //const Matrix2D &rMatQ = calculMatQ(vecH);

    // Calcul de la transposée QT de la matrice Q, de dimension (n-2) x n
    //const Matrix2D &rMatQT = transpose(rMatQ, 3);

    MatrixDiag diagWInv (sortedEvents.size());
    std::transform(sortedEvents.begin(), sortedEvents.end(), diagWInv.begin(), [](std::shared_ptr<Event> ev) {return 1.0L /ev->mW;});

    spline_matrices.diagWInv = diagWInv;
    // Calcul de la matrice matQTW_1Q, de dimension (n-2) x (n-2) pour calcul Mat_B
    // matQTW_1Q possèdera 3+3-1=5 bandes
    const Matrix2D &tmp = multiMatParDiag(spline_matrices.matQT, diagWInv, 3);

    //Matrix2D matQTW_1Qb = multiMatParMat(tmp, matQ, 3, 3);
    spline_matrices.matQTW_1Q = multiplyMatrixBanded_Winograd(tmp, spline_matrices.matQ, 1);

    // Calcul de la matrice QTQ, de dimension (n-2) x (n-2) pour calcul Mat_B
    // Mat_QTQ possèdera 3+3-1=5 bandes
    //Matrix2D matQTQb = multiMatParMat(matQT, matQ, 3, 3);
    //const Matrix2D &rMatQTQ = multiplyMatrixBanded_Winograd(rMatQT, rMatQ, 1);

    return spline_matrices;
}

SplineMatrices prepareCalculSpline_Sy2(const std::vector<std::shared_ptr<Event>> &sortedEvents, const std::vector<t_reduceTime> &vecH)
{
    const Matrix2D &rMatR = calculMatR(vecH);
    const Matrix2D &rMatQ = calculMatQ(vecH);

    // Calcul de la transposée QT de la matrice Q, de dimension (n-2) x n
    const Matrix2D &rMatQT = transpose(rMatQ, 3);

    MatrixDiag diagWInv (sortedEvents.size());
    std::transform(sortedEvents.begin(), sortedEvents.end(), diagWInv.begin(), [](std::shared_ptr<Event> ev) {return ev->mSy * ev->mSy ;});

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

SplineResults do_spline(const std::function <t_matrix (std::shared_ptr<Event>)> &fun, const SplineMatrices &matrices, const std::vector<std::shared_ptr<Event>> &events, const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp, const double lambdaSpline)
{
    const size_t n = events.size();
    std::vector<t_matrix> vec_Y (n);
    std::transform(events.begin(), events.end(), vec_Y.begin(), fun);

    return do_spline(vec_Y, matrices, vecH, decomp, lambdaSpline);

    /*
    * MatB doit rester en copie
    */

    /*    SplineResults spline;
    try {
        // calcul de: R + lambda * Qt * W-1 * Q = Mat_B


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
        const decltype(SplineResults::vecGamma) &vecGamma = resolutionSystemeLineaireCholesky(decomp, vecQtY);

        // Calcul du vecteur g = Y - lambda * W-1 * Q * gamma
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
    */
}


SplineResults doSplineX(const SplineMatrices &matrices, const std::vector<std::shared_ptr<Event> > &events, const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp, const double lambdaSpline)
{
    return do_spline(get_Yx, matrices, events, vecH, decomp, lambdaSpline);
}

SplineResults doSplineY(const SplineMatrices &matrices, const std::vector<std::shared_ptr<Event>> &events, const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag> &decomp, const double lambdaSpline)
{
    return do_spline(get_Yy, matrices, events, vecH, decomp, lambdaSpline);
}

SplineResults doSplineZ(const SplineMatrices &matrices, const std::vector<std::shared_ptr<Event> > &events, const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, MatrixDiag > &decomp, const double lambdaSpline)
{
    return do_spline(get_Yz, matrices, events, vecH, decomp, lambdaSpline);
}

/**
 Cette procedure calcule la matrice inverse de B:
 B = R + lambda * Qt * W-1 * Q
 puis calcule la matrice d'influence A(lambda)
 Bande : nombre de diagonales non nulles
 used only with MCMCLoopCurve::calcul_spline_variance()
*/
std::vector<t_matrix> calculMatInfluence_origin(const SplineMatrices& matrices, const int nbBandes, const std::pair<Matrix2D, MatrixDiag> &decomp, const double lambda)
{
    // Q_ASSERT_X(lambda!=0, "[CurveUtilities calculMatInfluence_origin]", "lambda=0");
    /*
     * if (lambda == 0)
     * return matA.resize(n, 1.);
     */
    const size_t n = matrices.diagWInv.size();

    const Matrix2D &matB_1 = inverseMatSym_origin(decomp, nbBandes + 4, 1);

    std::vector<t_matrix> matQB_1QT;
    matQB_1QT.reserve(n);

    /*Calcul des termes diagonaux de Q (matB_1) Qt, de i=1 à nb_noeuds
             ??? vérifier le contrôle des indices i+1: débordement de matrices */


    const auto* matQi = begin(matrices.matQ[0]); // matQ est une matrice encadrée de 0
    // long double term = pow(matQi[0], 2.0) * matB_1[0][0]; // matQ[0][0] ici vaut toujours 0
    // term += pow(matQi[1], 2.0) * matB_1[1][1];
    // term += 2.0 * matQi[0] * matQi[1] * matB_1[0][1]; // ici vaut toujours 0


    t_matrix term = std::pow(matQi[0], 2.0L) * matB_1[0][0];
    term += std::pow(matQi[1], 2.0L) * matB_1[1][1];
    term += 2.0L * matQi[0] * matQi[1] * matB_1[0][1];


    matQB_1QT.push_back(term);

    for (size_t i = 1; i < n-1; ++i) {
        matQi = begin(matrices.matQ[i]);

        t_matrix term_i = std::pow(matQi[i-1], 2.0L) * matB_1[i-1][i-1];
        term_i += std::pow(matQi[i], 2.0L) * matB_1[i][i];
        term_i += std::pow(matQi[i+1], 2.0L) * matB_1[i+1][i+1];

        term_i += 2.0L * matQi[i-1] * matQi[i] * matB_1[i-1][i];
        term_i += 2.0L * matQi[i-1] * matQi[i+1] * matB_1[i-1][i+1];
        term_i += 2.0L * matQi[i] * matQi[i+1] * matB_1[i][i+1];


        matQB_1QT.push_back(term_i);
    }

    matQi = begin(matrices.matQ[n-1]);

    term = std::pow(matQi[n-2], 2.0L) * matB_1[n-2][n-2];
    term += std::pow(matQi[n-1], 2.0L) * matB_1[n-1][n-1];
    term += 2.0L * matQi[n-2] * matQi[n-1] * matB_1[n-2][n-1];

    matQB_1QT.push_back(term);

    // Multi_diag_par_Mat(Diag_W_1c, Mat_QB_1QT, Nb_noeudsc, 1, tmp1); // donne une matrice, donc la diagonale est le produit des diagonales
    // Multi_const_par_Mat(-alphac, tmp1, Nb_noeudsc,1, Mat_Ac);
    // Addit_I_et_Mat(Mat_Ac,Nb_noeudsc);
    // remplacé par:

    std::vector<t_matrix> matA;
    matA.reserve(n);

    long double lambda_L = static_cast<long double>(lambda);
    for (size_t i = 0; i < n; ++i) {
        long double mat_a = 1.0L - lambda_L * static_cast<long double>(matrices.diagWInv[i]) * matQB_1QT[i];
        matA.push_back(std::clamp(mat_a, 0.0L, 1.0L));

#if DEBUG
       // const double mA = 1.0 - lambda * (double)matrices.diagWInv[i] * matQB_1QT[i];
        if (mat_a == 0.0) {
            qWarning ("[MCMCLoopCurve::calculMatInfluence_origin] -> Oups matA.at(i) == 0 ");
        }

       /* if (mat_a < 0.0) {
            qDebug ()<<"[MCMCLoopCurve::calculMatInfluence_origin] -> Oups matA.at(i) ="<< matA[i] << " < 0  ; LambdaSpline="<<lambda;

        }

        if (mat_a > 1.0) {
            qWarning ("[MCMCLoopCurve::calculMatInfluence_origin] -> Oups matA.at(i) > 1  ");

        }*/
#endif
    }

    return matA;
}


std::vector<double> calcul_spline_variance(const SplineMatrices& matrices, const std::vector<std::shared_ptr<Event>> &events, const std::pair<Matrix2D, MatrixDiag> &decomp, const double lambdaSpline)
{
    unsigned int n = (unsigned int)events.size();
    std::vector<t_matrix> matA = calculMatInfluence_origin(matrices, 1, decomp, lambdaSpline);
    std::vector<double> varG;

    for (unsigned int i = 0; i < n; ++i) {
#ifdef DEBUG
        const double& aii = matA[i];
        // si Aii négatif ou nul, cela veut dire que la variance sur le point est anormalement trop grande,
        // d'où une imprécision dans les calculs de Mat_B (Cf. calcul spline) et de mat_A
        if (aii < 0. || events[i]->mW <0) {
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
MCMCSpline currentSpline (std::vector<std::shared_ptr<Event> > &events, const std::vector<t_reduceTime> &vecH, const SplineMatrices &matrices, const double lambda, bool doY, bool doZ)
{

    const auto vec_theta_red = get_vector<t_reduceTime>(get_ThetaReduced, events);

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

    for (size_t i = 0; i < events.size(); i++) {
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
        for (size_t i = 0; i < events.size(); i++) {
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
        for (size_t i = 0; i < events.size(); i++) {
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
 * @return
 */
MCMCSpline currentSpline_WI (std::vector<std::shared_ptr<Event>> &events, bool doY, bool doZ, bool use_error)
{
    //Q_ASSERT_X(mModel->mLambdaSpline.mX!=0, "[MCMCLoopCurve::ln_h_YWI_3_update]", "lambdaSpline=0");

    const std::vector<t_reduceTime> &vecH = calculVecH(events);
    const SplineMatrices &spline_matrices = prepare_calcul_spline_WI(vecH);


    const auto &vec_theta_red = get_vector<t_reduceTime>(get_ThetaReduced, events);

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
    const unsigned n = (unsigned) spline.vecThetaReduced.size();
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
    const unsigned n = (unsigned)spline.vecThetaReduced.size();
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

double valeurGSeconde(const double t, const MCMCSplineComposante& spline, Model& model)
{
    const size_t n = spline.vecThetaReduced.size();
    const auto tReduce = model.reduceTime(t);
    // The second derivative is always zero outside the interval [t1,tn].
    double gSeconde = 0.;

    for (size_t i = 0; i < n-1; ++i) {
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
#ifdef DEBUG
                if (std::isnan(varG))
                    qDebug()<< "[CurveUtilities] varG is nan ??"<<ti1<<ti2;
#endif
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

std::vector<t_matrix> do_vec_gamma(const std::vector<t_matrix>& vec_Y, const std::vector<t_reduceTime>& vec_H, const std::pair<Matrix2D, MatrixDiag>& decomp)
{
    size_t N = vec_Y.size();

    std::vector<t_matrix> vecQtY;
    vecQtY.push_back(0.0);
    for (size_t i = 1; i < N-1; ++i) {
        const t_matrix term1 = (vec_Y[i+1] - vec_Y[i]) / vec_H[i];
        const t_matrix term2 = (vec_Y[i] - vec_Y[i-1]) / vec_H[i-1];
        vecQtY.push_back(term1 - term2);
    }
    vecQtY.push_back(0.0);

    return resolutionSystemeLineaireCholesky(decomp, vecQtY);
}

// Calcul du vecteur g = Y - lambda * W-1 * Q * gamma
// tout est calculé en t_matrix (càd long double)
// Si lambda =0, il est préférable de faire directement vec_G=vec_Y
std::vector<t_matrix> do_vec_G(const SplineMatrices& matrices, const std::vector<t_matrix> &vec_Gamma, const std::vector<t_matrix>& vec_Y, const double lambdaSpline)
{
    const size_t n = vec_Y.size();
    std::vector<t_matrix> vec_G;
    vec_G.reserve(n);
    if (lambdaSpline != 0) {
        const std::vector<t_matrix>& Q_gamma = multiMatParVec(matrices.matQ, vec_Gamma, 3);
        const MatrixDiag& W_Inv = matrices.diagWInv;
        const t_matrix lambda = lambdaSpline;

        for (unsigned i = 0; i < n; ++i) {
            t_matrix ldt = lambda * W_Inv[i] * Q_gamma[i];
            t_matrix g = vec_Y[i] - ldt;

            if (std::isnan(g)) {
                // qDebug()<< " isnan(g)";
                vec_G.push_back( +INFINITY) ;
            } else {
                vec_G.push_back( g) ;
            }

        }

    } else {
        vec_G = vec_Y;
    }

    return vec_G;
}

std::vector<double> calcul_spline_variance(const SplineMatrices& matrices, const std::pair<Matrix2D, MatrixDiag>& decomp, const double lambda)
{
    const std::vector<t_matrix>& matA = calculMatInfluence_origin(matrices, 1, decomp, lambda);
    std::vector<double> varG;
    const auto& W_Inv = matrices.diagWInv;
    size_t n = W_Inv.size();

    for (size_t i = 0; i < n; ++i) {
        varG.push_back(static_cast<double>(matA[i]  * W_Inv[i]));
    }

    return varG;
}

/**
 * @brief composante_to_curve, used for fitPlot()
 * @param spline_compo
 * @param tmin
 * @param tmax
 * @param step
 * @return
 */
std::vector<QMap<double, double>> composante_to_curve(MCMCSplineComposante spline_compo, double tmin, double tmax, double step)
{
    QMap<double, double> curve;
    QMap<double, double> curve_plus;
    QMap<double, double> curve_moins;
    double g = 0.;
    double varG = 0.;
    double gp = 0.;
    double gs = 0.;
    int nb_pts = (tmax-tmin)/step + 1;
    unsigned i0 = 0;
    for (int i= 0; i < nb_pts ; ++i) {
        const double t = static_cast<double>(i) * step + tmin ;
        valeurs_G_VarG_GP_GS(t, spline_compo, g, varG, gp, gs, i0, tmin, tmax);
        curve[t] = g;
        curve_plus[t] = g + 1.96 * sqrt(varG);
        curve_moins[t] = g - 1.96 * sqrt(varG);
    }
    return std::vector<QMap<double, double>>({curve, curve_plus, curve_moins});
}


/**
 * @brief MCMCLoopCurve::doSpline
 * Fabrication de spline avec
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
SplineResults do_spline(const std::vector<t_matrix>& vec_Y, const SplineMatrices& matrices, const std::vector<t_reduceTime>& vecH, const std::pair<Matrix2D, MatrixDiag>& decomp, const double lambdaSpline)
{
    SplineResults spline;
    try {
        // calcul de: R + alpha * Qt * W-1 * Q = Mat_B
        // transtypage de vec_Y
        //std::vector<t_matrix> vec_Y_L(vec_Y.begin(), vec_Y.end());

        // Calcul du vecteur Gamma
        const std::vector<t_matrix>& vec_gamma_L = do_vec_gamma(vec_Y, vecH, decomp);

        // calcul de vecG
        if (lambdaSpline != 0.0) {
            std::vector<t_matrix> vec_G_L = do_vec_G(matrices, vec_gamma_L, vec_Y, lambdaSpline);
            spline.vecG = std::vector<double>(vec_G_L.begin(), vec_G_L.end());

        } else {
            spline.vecG = std::vector<double> (vec_Y.begin(), vec_Y.end());//vec_Y;
        }

        spline.vecGamma = std::vector<double>(vec_gamma_L.begin(), vec_gamma_L.end());


    } catch(...) {
        qCritical() << "[do_Spline] : Caught Exception!\n";
    }

    return spline;
}


std::pair<MCMCSpline, std::pair<double, double>> do_spline_composante(const std::vector<double> &vec_t, const std::vector<double> &vec_X, const std::vector<double> &vec_X_err, double tmin, double tmax, SilvermanParam &sv, const std::vector<double> &vec_Y, const std::vector<double> &vec_Y_err, const std::vector<double> &vec_Z, const std::vector<double> &vec_Z_err)
{

    bool doY (!vec_Y.empty() && vec_Z.empty());
    bool doYZ (!vec_Y.empty() && !vec_Z.empty());

    std::vector<t_matrix> vec_tmp_t;
    std::vector<t_matrix> vec_tmp_x, vec_tmp_y, vec_tmp_z;
    std::vector<t_matrix> vec_tmp_x_err, vec_tmp_y_err, vec_tmp_z_err;
    // trie des temps et des données associées
    if (!std::is_sorted(vec_t.begin(), vec_t.end())) {
        std::vector<int> l_index = get_order(vec_t);

        for (int i : l_index) {
            vec_tmp_t.push_back(vec_t.at(i));
            vec_tmp_x.push_back(vec_X.at(i));
            vec_tmp_x_err.push_back(vec_X_err.at(i));
            if (doY || doYZ) {
                vec_tmp_y.push_back(vec_Y.at(i));
                vec_tmp_y_err.push_back(vec_Y_err.at(i));
            }
            if (doYZ) {
                vec_tmp_z.push_back(vec_Z.at(i));
                vec_tmp_z_err.push_back(vec_Z_err.at(i));
            }
        }
    } else {
        vec_tmp_t = std::vector<t_matrix>(vec_t.begin(), vec_t.end()); //vec_t;
        vec_tmp_x = std::vector<t_matrix>(vec_X.begin(), vec_X.end()); //vec_X;
        vec_tmp_x_err = std::vector<t_matrix>(vec_X_err.begin(), vec_X_err.end()); //vec_X_err;
        if (doY || doYZ) {
            vec_tmp_y = std::vector<t_matrix>(vec_Y.begin(), vec_Y.end());
            vec_tmp_y_err = std::vector<t_matrix>(vec_Y_err.begin(), vec_Y_err.end()); //vec_Y_err;
        }
        if (doYZ) {
            vec_tmp_z = std::vector<t_matrix>(vec_Z.begin(), vec_Z.end()); //vec_Z;
            vec_tmp_z_err = std::vector<t_matrix>(vec_Z_err.begin(), vec_Z_err.end()); //vec_Z_err;
        }
    }

    std::vector<t_reduceTime> vecH;
    std::vector<t_reduceTime> vec_theta_red;

    for (auto t : vec_tmp_t) {
        vec_theta_red.push_back((t-tmin)/(tmax-tmin));
        //qDebug()<<"do spline compo t="<< t;
    }

    spread_theta_reduced(vec_theta_red);

    auto iter = vec_theta_red.begin();
    for (; iter != std::prev(vec_theta_red.end()) ; iter++) {
        vecH.push_back(*std::next(iter) -  *iter);
    }
    // doSpline utilise les Y des events
    // => On le calcule ici pour la première composante (x)

    double lambda_cv, Vg;
    std::pair<double, double> lambda_Vg;
    if (sv.lambda_fixed == false) {
        lambda_Vg = initLambdaSplineBySilverman(sv, vec_tmp_x, vec_tmp_x_err, vecH, vec_tmp_y, vec_tmp_y_err, vec_tmp_z, vec_tmp_z_err);
        lambda_cv = lambda_Vg.first;

        Vg = lambda_Vg.second;

    } else {
        lambda_cv = pow(10., sv.log_lambda_value);

        std::vector<long double> W_1;
        for (auto X_err : vec_tmp_x_err) {
            W_1.push_back(pow(X_err, 2.0));
        }
        const SplineMatrices& spline_matrices_tmp = prepare_calcul_spline(vecH, W_1);
        Vg = var_residual(vec_tmp_x, spline_matrices_tmp, vecH, lambda_cv);

        if (doY) {
            Vg += var_residual(vec_tmp_y, spline_matrices_tmp, vecH, lambda_cv);
            Vg /= 2.;
        } else if (doYZ) {
            Vg += var_residual(vec_tmp_y, spline_matrices_tmp, vecH, lambda_cv) + var_residual(vec_tmp_z, spline_matrices_tmp, vecH, lambda_cv);
            Vg /= 3.;
        }


    }
    lambda_Vg = std::make_pair(lambda_cv, Vg);
    qDebug()<<" end of cross_validation lambda : "<<lambda_cv<<" = 10E"<<log10(lambda_cv)<< " Vg="<< Vg <<"sqrt(Vg)"<<sqrt(Vg);

    std::vector<t_matrix> W_1;
    if (sv.use_error_measure) {
        if (doY) {
            auto Y_err = vec_tmp_y_err.begin();
            for (auto X_err : vec_tmp_x_err) {
                const double Sy = pow(X_err, -2.) + pow(*Y_err++, -2.) ;
                W_1.push_back(2./Sy);
            }
        } else if (doYZ) {
            auto Y_err = vec_tmp_y_err.begin();
            auto Z_err = vec_tmp_z_err.begin();
            for (auto X_err : vec_tmp_x_err) {
                const double Sy = pow(X_err, -2.) + pow(*Y_err++, -2.) + pow(*Z_err++, -2.) ;
                W_1.push_back(3./Sy);
            }
        } else {

            for (auto X_err : vec_tmp_x_err) {
                W_1.push_back(pow(X_err, 2.));
            }
        }

    } else {
        W_1 = std::vector<t_matrix>(vec_tmp_x_err.size(), 1.0L);
    }

    const SplineMatrices& spline_matrices = prepare_calcul_spline(vecH, W_1);


  //  Spline final

    std::unique_ptr<std::pair<Matrix2D, MatrixDiag>> decomp;

    if (lambda_cv != 0.0) {
        decomp = std::make_unique<std::pair<Matrix2D, MatrixDiag>>(decomp_matB(spline_matrices, lambda_cv));

    } else {
        decomp = std::make_unique<std::pair<Matrix2D, MatrixDiag>>(decompositionCholesky(spline_matrices.matR, 5, 1));
    }


    const SplineResults& sx = do_spline(vec_tmp_x, spline_matrices,  vecH, *decomp, lambda_cv);

    std::vector<double> varG;

    if (lambda_cv == 0) {
        if (sv.use_error_measure) {
            varG = std::vector<double>(W_1.begin(), W_1.end());

        } else {
            varG =  std::vector<double>(W_1.size(), 0.0);
        }


    } else {
        const std::vector<t_matrix>& matA = calculMatInfluence_origin(spline_matrices, 1, *decomp, lambda_cv);

        // Affectation de Vg pour l'affichage de l'erreur global
        for (auto ai : matA)
            varG.push_back(ai*Vg);
    }


    // Cas Erreur ré-évaluée suivant Silverman, Some aspect..., page 7
    /*
    int n = matA.size();
    std::vector<double> general_resi = general_residual(vec_X, spline_matrices, vecH, lambda_cv);

    for (int i = 0; i < n; ++i) {

        const int k = 5; // Dans l'article utilisation de la valeur 5 ?? à addapter suivant les modèles
        const int mi =  (i-k<0 ? 0: i-k);// std::max(0, i-k);
        const int ni =  (i+k<n ? i+k : n);
        double reestimat_W_1 = 0.;
        for (int j = mi; j< ni; j++) {
            reestimat_W_1 += pow(general_resi[j], 2);
        }
        reestimat_W_1 *= spline_matrices.diagWInv[i];
        reestimat_W_1 /= (ni-mi);

        varG.push_back(matA[i]  * reestimat_W_1);

    }
    */
    // --------------------------------------------------------------
    //  Calcul de la spline g, g" pour chaque composante x y z + stockage
    // --------------------------------------------------------------
    MCMCSpline spline;

    MCMCSplineComposante splineX, splineY, splineZ;

    splineX.vecThetaReduced = vec_theta_red;
    splineX.vecG = std::move(sx.vecG);
    splineX.vecGamma = std::move(sx.vecGamma);

    splineX.vecVarG = varG;

    /*showVector(splineX.vecThetaReduced, "do_spline_composante splineX.vecThetaReduced");
    showVector(splineX.vecG, "splineX.vecG");
    showVector(splineX.vecGamma, "splineX.vecGamma");
    showVector(splineX.vecVarG, "splineX.vecVarG");
*/

    if (doY || doYZ) {
        const SplineResults &sy = do_spline(vec_tmp_y, spline_matrices,  vecH, *decomp, lambda_cv);

        splineY.vecThetaReduced = vec_theta_red;
        splineY.vecG = std::move(sy.vecG);
        splineY.vecGamma = std::move(sy.vecGamma);

        splineY.vecVarG = varG;

        if (doYZ) {
            const SplineResults &sz = do_spline(vec_tmp_z, spline_matrices,  vecH, *decomp, lambda_cv);

            splineZ.vecThetaReduced = vec_theta_red;
            splineZ.vecG = std::move(sz.vecG);
            splineZ.vecGamma = std::move(sz.vecGamma);

            splineZ.vecVarG = varG;
        }
    }

    spline.splineX = std::move(splineX);
    spline.splineY = std::move(splineY);
    spline.splineZ = std::move(splineZ);

    return std::make_pair( spline, lambda_Vg);
}


long double cross_validation (const std::vector<t_matrix>& vec_Y, const SplineMatrices& matrices, const std::vector<t_reduceTime>& vecH, const double lambda)
{

    const long double N = matrices.diagWInv.size();

    Matrix2D matB = addMatEtMat(matrices.matR, multiConstParMat(matrices.matQTW_1Q, lambda, 5), 5);

    // Decomposition_Cholesky de matB en matL et matD
    // Si alpha global: calcul de Mat_B = R + alpha * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
    const auto &decomp = decompositionCholesky(matB, 5, 1);

    //const SplineResults& s = do_spline(vec_Y, matrices,  vecH, decomp, lambda);

    // On calcul la spline g en type t_matrix= long double pour plus de précision

    const std::vector<t_matrix>& vec_gamma_L = do_vec_gamma(vec_Y, vecH, decomp);

    const std::vector<t_matrix>& g = do_vec_G(matrices, vec_gamma_L, vec_Y, lambda);

    const std::vector<t_matrix>& matA = calculMatInfluence_origin(matrices, 1, decomp, lambda);

    long double CV = 0.0;
    for (int i = 0 ; i < N; i++) {
        CV +=  pow((g[i] - vec_Y[i]) / (1.0 - matA[i]), 2.0)/ matrices.diagWInv[i];
    }

    return CV/N;
}

// Algorithme de Kahan pour somme compensée (améliore la précision)
/*static long double compensated_sum(const std::vector<double>& values) {
    long double sum = 0.0L;
    long double c = 0.0L;  // Terme de compensation

    for (const double& val : values) {
        long double y = static_cast<long double>(val) - c;
        long double t = sum + y;
        c = (t - sum) - y;
        sum = t;
    }

    return sum;
}
*/

// Algorithme de Kahan pour somme compensée (améliore la précision)
long double compensated_sum(const std::vector<long double>& values)
{
    long double sum = 0.0L;
    long double c = 0.0L;  // Terme de compensation

    for (const long double& val : values) {
        long double y = val - c;
        long double t = sum + y;
        c = (t - sum) - y;
        sum = t;
    }

    return sum;
}
/**
 * @brief Calculates the Generalized Cross-Validation (GCV) score for a spline model.
 * i.e. the prediction of $(g_(ti)-Yij)$ without the point Yij. With weight in matrices
 *
 * This function computes the GCV score, which is used to select the optimal
 * smoothing parameter (lambda) in spline regression. The GCV score is calculated
 * using the observed data (vec_Y), the estimated values (s.vecG), and the influence
 * of each data point (matA).
 *
 * The formula for GCV is:
 * \f[
 * GCV = \frac{\sum_{i=1}^{N} \frac{(Y_i - \hat{G}_i)^2}{W_{ii}}}{(N(1 - \frac{\text{trace}(A)}{N}))^2}
 * \f]
 * where:
 * - \( Y_i \) is the observed value
 * - \( \hat{G}_i \) is the predicted value
 * - \( W_{ii} \) is the diagonal element of the weight matrix (inverse of the precision)
 * - \( N \) is the number of observations
 * - \( A \) is the influence matrix
 * - \( \text{trace}(A) \) is the sum of the diagonal elements of matrix \( A \)
 * - \( DLEc = 1 - \frac{\text{trace}(A)}{N} \) is the degrees of freedom correction
 *
 * @param vec_Y Vector of observed values (dependent variable)
 * @param matrices Struct containing matrices used in spline calculations
 * @param vecH Vector related difference of time
 * @param lambda Smoothing parameter
 * @return double The computed GCV score
 */
long double general_cross_validation (const std::vector<t_matrix> &vec_Y, const SplineMatrices& matrices, const std::vector<t_reduceTime> &vecH, const double lambda)
{
    const double N = matrices.diagWInv.size();

    // Create matB = R + lambda * Qt * W-1 * Q

    // Perform Cholesky decomposition of matB

    const auto& decomp = decomp_matB(matrices, lambda);


    const std::vector<t_matrix>& vec_gamma_L = do_vec_gamma(vec_Y, vecH, decomp);

    const std::vector<t_matrix>& g = do_vec_G(matrices, vec_gamma_L, vec_Y, lambda);

    // Calculate the influence matrix matA
    const std::vector<t_matrix>& matA = calculMatInfluence_origin(matrices, 1, decomp, lambda);

    //long double long_GCV = 0.0L;
    const long double long_trace = compensated_sum(matA) / static_cast<long double>(N);
    const long double long_DLEc = 1.0L - long_trace;

/*
    for (size_t i = 0; i < vec_Y_L.size(); ++i) {
        long double residual = g[i] - vec_Y_L[i];
        long double weight_inv = matrices.diagWInv[i];

        long_GCV += residual * residual / weight_inv;
    }
  */
    // Utilisation de l'algorithme de Kahan pour la sommation
    long double sum = 0.0L;
    long double c = 0.0L;  // Variable de compensation

    for (size_t i = 0; i < vec_Y.size(); ++i) {
        long double residual = g[i] - vec_Y[i];
        long double weight_inv = matrices.diagWInv[i];

        // Calculer (residual * residual / weight_inv) avec ordre optimal d'opérations
        long double term = (residual / std::sqrt(weight_inv)) * (residual / std::sqrt(weight_inv));

        // Sommation de Kahan
        long double y = term - c;
        long double t = sum + y;
        c = (t - sum) - y;
        sum = t;
    }


    sum /= static_cast<long double>(N);
    // GCV /= N/sum_wi; // ici ?? C'est une constante dans la recherche
    sum /= long_DLEc * long_DLEc;
    //

    return sum;
}



double RSS(const std::vector<t_matrix> &vec_Y, const SplineMatrices &matrices, const std::vector<t_reduceTime> &vecH, const double lambda)
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

    const SplineResults& sy = do_spline(vec_Y, matrices,  vecH, decomp, lambda);

    const std::vector< double>& vecG = sy.vecG;

    double res = 0.0;

    auto g = vecG.begin();
    for (auto& Y : vec_Y) {
        res += pow(Y - *g++, 2.0);
    }

    return std::move(res);

}

double var_Gasser(const std::vector<double>& vec_t, const std::vector<double>& vec_Y)
{
    double N = vec_t.size();
    double sum = 0.0;
    double c = 0.0; // Compensation

    for (auto i=1; i< N-1; i++) {
        double di = vec_t[i+1] - vec_t[i-1];
        double ai = di > 0 ?(vec_t[i+1] - vec_t[i]) / di : 0.5;
        double bi = 1.0 - ai;

        double ei = ai * vec_Y[i-1] + bi * vec_Y[i+1] - vec_Y[i];
        double ci2 = 1.0 / (1.0 + ai*ai + bi*bi);

        double term = ci2 * ei * ei;
        const double y = term - c;
        const double t = sum + y;
        c = (t - sum) - y;
        sum = t;

    }

    return sum / (N - 2.0);
}

long double var_Gasser(const std::vector<long double>& vec_t, const std::vector<long double>& vec_Y)
{
    const size_t N = vec_t.size();
    if (N <= 2) return 0.0L; // Protection contre les cas limites

    // Utilisation de la sommation de Kahan pour améliorer la précision
    long double sum = 0.0L;
    long double c = 0.0L; // Variable de compensation

#pragma omp parallel for reduction(+:sum) // Parallélisation si OpenMP est disponible
    for (size_t i = 1; i < N - 1; ++i) {
        const long double di = vec_t[i+1] - vec_t[i-1];
        const long double ai = di > 0.0L ? (vec_t[i+1] - vec_t[i]) / di : 0.5L;
        const long double bi = 1.0L - ai;

        const long double ei = ai * vec_Y[i-1] + bi * vec_Y[i+1] - vec_Y[i];

        const long double ci2 = 1.0L / (1.0L + ai*ai + bi*bi);
        const long double term = ci2 * ei * ei;

// Sommation de Kahan (si non parallélisé)
#ifndef _OPENMP
        const long double y = term - c;
        const long double t = sum + y;
        c = (t - sum) - y;
        sum = t;
#else
        sum += term;
#endif
    }

    return sum / static_cast<long double>(N - 2);
}

// Cross-validation initialization
std::pair<double, double> initLambdaSplineByCV(const bool depth, const std::vector<t_matrix> &vec_X, const std::vector<t_matrix> &vec_X_err, const SplineMatrices &matrices, const std::vector<t_reduceTime> &vecH,const std::vector<t_matrix> &vec_Y, const std::vector<t_matrix> &vec_Z)
{
    std::vector<double> GCV, CV, lambda_GCV, lambda_CV, lambda_GCV_Vg, lambda_CV_Vg;

    double Vg;
    bool doY = !vec_Y.empty() && vec_Z.empty();
    bool doYZ = !vec_Y.empty() && !vec_Z.empty();

    for (int idxLambda = -200; idxLambda < 101; ++idxLambda ) {
        const double lambda_loop = pow(10., ( double)idxLambda/10.);

        Vg = var_residual(vec_X, matrices, vecH, lambda_loop);

        if (doY) {
            Vg += var_residual(vec_Y, matrices, vecH, lambda_loop);
            Vg /= 2.;

        } else if (doYZ) {
            Vg += var_residual(vec_Y, matrices, vecH, lambda_loop) + var_residual(vec_Z, matrices, vecH, lambda_loop);
            Vg /= 3.;
        }


        std::vector<long double> vec_Vg_Si2;

        for (auto X_err : vec_X_err) {
            vec_Vg_Si2.push_back(Vg + pow(X_err, 2.));
        }

        const SplineMatrices &test_matrices = prepare_calcul_spline(vecH, vec_Vg_Si2);



        //-----
        const double cv = cross_validation(vec_X, test_matrices, vecH, lambda_loop);
        const double gcv = general_cross_validation(vec_X, test_matrices, vecH, lambda_loop);


        if (!std::isnan(gcv)) { // && !std::isinf(gcv)) {
            GCV.push_back(gcv);
            lambda_GCV.push_back(lambda_loop);
            lambda_GCV_Vg.push_back(Vg);
            //qDebug()<<" gcv="<<idxLambda<<GCV.back();//<< has_positif;
        }

        if (!std::isnan(cv)) {// && !std::isinf(cv)) {
            CV.push_back(cv);
            lambda_CV.push_back(lambda_loop);
            lambda_CV_Vg.push_back(Vg);
            //qDebug()<<" cv="<<idxLambda<<CV.back();//<< has_positif;
        }
    }

    // We are looking for the smallest CV value
    unsigned long idxDifMin = std::distance(GCV.begin(), std::min_element(GCV.begin(), GCV.end()) );

    unsigned long idxDifMinCV = std::distance(CV.begin(), std::min_element(CV.begin(), CV.end()) );


    // If the mini is at one of the bounds, there is no solution in the interval for GCV
    // See if there is a solution in CV


    std::vector<double>* lambda_vect;
    std::vector<double>* Vg_vect;
    int idx_vect;

     if (idxDifMin == 0 || idxDifMin == (GCV.size()-1)) {
        qDebug()<<" 2d chance\t idxDifMin CV ="<<idxDifMinCV<< "Pas de solution avec GCV idxDifMin GCV = 00 "<<idxDifMin;

        if (idxDifMinCV == 0 || idxDifMinCV == (CV.size()-1)) {
            qDebug()<<"[initLambdaSplineByCV] No solution with CV ="<<idxDifMinCV<< " On prend lambda_GCV[0] = 10E"<<log10(lambda_GCV.at(0));

            //lambda_final = lambda_GCV.at(0);
            //Vg = lambda_GCV_Vg.at(0);

            idx_vect = 0;
            lambda_vect = &lambda_GCV;
            Vg_vect = &lambda_GCV_Vg;

        } else {
            qDebug()<<"[initLambdaSplineByCV] With CV lambda_CV.at("<<idxDifMinCV<<")= 10E"<<log10(lambda_CV.at(idxDifMinCV));

            idx_vect = idxDifMinCV;
            lambda_vect = &lambda_CV;
            Vg_vect = &lambda_CV_Vg;
        }

     } else {
        qDebug()<<"[initLambdaSplineByCV] With GCV lambda_GCV.at("<<idxDifMin<<")= 10E"<<log10(lambda_GCV.at(idxDifMin));

        idx_vect = idxDifMin;
        lambda_vect = &lambda_GCV;
        Vg_vect = &lambda_GCV_Vg;
     }


     // test positif

     if (depth) {

        bool has_positif = false;

        int idx_test = idx_vect;
        double lambda_test, Vg_test;
        do {
            lambda_test =  lambda_vect->at(idx_test);
            Vg_test = Vg_vect->at(idx_test);

            std::vector<long double> vec_Vg_Si2;
            for (auto X_err : vec_X_err) {
                vec_Vg_Si2.push_back(Vg_test + pow(X_err, 2.));
            }
            const SplineMatrices &spline_matrices = prepare_calcul_spline(vecH, vec_Vg_Si2);

            //  Spline final

            const Matrix2D &tmp = multiConstParMat(spline_matrices.matQTW_1Q, lambda_test, 5);
            const Matrix2D &matB = addMatEtMat(spline_matrices.matR, tmp, 5);


            // Decomposition_Cholesky de matB en matL et matD
            // Si alpha global: calcul de Mat_B = R + alpha * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
            const std::pair<Matrix2D, MatrixDiag> &decomp = decompositionCholesky(matB, 5, 1);

            const SplineResults &sx = do_spline(vec_X, spline_matrices,  vecH, decomp, lambda_test);

            const std::vector<double> &vecVarG = calcul_spline_variance(spline_matrices, decomp, lambda_test);

            // recomposition de vec_theta_red
            std::vector<double> vec_theta_red;
            double som = 0;
            vec_theta_red.push_back(som);
            for (auto& h : vecH) {
                som += h;
                vec_theta_red.push_back(som);
            }


            MCMCSplineComposante splineX;
            splineX.vecThetaReduced = vec_theta_red;
            splineX.vecG = std::move(sx.vecG);
            splineX.vecGamma = std::move(sx.vecGamma);

            splineX.vecVarG = vecVarG;

            /*for (int i = 0; i < events.size(); i++) {
            events[i]->mGx = splineX.vecG[i];
            }*/

            MCMCSpline spline;
            spline.splineX = std::move(splineX);

            has_positif = hasPositiveGPrimeByDet(spline.splineX);
            if (!has_positif) {
                idx_test++;
                if (idx_test > (int)Vg_vect->size()-1) {
                   has_positif = true;
                }
            }

        } while (!has_positif && lambda_test<1E10);

        //------
        return std::make_pair(lambda_test, Vg_test);

    } else {

        return std::make_pair(lambda_vect->at(idx_vect), Vg_vect->at(idx_vect));
    }

    // ----- RETURN
    return std::make_pair(1., 0.);

}
#pragma mark Silvermann

std::pair<double, double> initLambdaSplineBySilverman(SilvermanParam& sv, const std::vector<t_matrix>& vec_X, const std::vector<t_matrix>& vec_X_err, const std::vector<t_reduceTime> &vecH, const std::vector<t_matrix> &vec_Y, const std::vector<t_matrix> &vec_Y_err, const std::vector<t_matrix> &vec_Z, const std::vector<t_matrix> &vec_Z_err)
{
    std::vector<long double> GCV, CV;
    std::vector<double> lambda_GCV, lambda_CV;
    const bool doY = !vec_Y.empty() && vec_Z.empty();
    const bool doYZ = !vec_Y.empty() && !vec_Z.empty();

    unsigned long first_mini_gcv_idx = 0;
    unsigned long first_mini_cv_idx = 0;


    std::vector<t_matrix> W_1;

    if (doY) {
        auto Y_err = vec_Y_err.begin();
        for (auto &X_err : vec_X_err) {
            const double Sy = pow(X_err, -2.0) + pow(*Y_err++, -2.0);
            W_1.push_back(2.0 / Sy);
        }
    }
    else if (doYZ) {
        auto Y_err = vec_Y_err.begin();
        auto Z_err = vec_Z_err.begin();
        for (auto &X_err : vec_X_err) {
            const double Sy =
                pow(X_err, -2.) + pow(*Y_err++, -2.0) + pow(*Z_err++, -2.0);
            W_1.push_back(3.0 / Sy);
        }
    }
    else {
        for (auto &X_err : vec_X_err) {
            W_1.push_back(pow(X_err, 2.0));
        }
    }

    const SplineMatrices& test_matrices = prepare_calcul_spline(vecH, W_1);

    for (int lambda_loop_exp = -1000; lambda_loop_exp < 1001; ++lambda_loop_exp ) {

        const double lambda_loop = pow(10.0, static_cast<double>(lambda_loop_exp)/100.0);
        long double cv = cross_validation(vec_X, test_matrices, vecH, lambda_loop);

        if (doY) {
            cv += cross_validation(vec_Y, test_matrices, vecH, lambda_loop);
            cv /= 2.0;

        }  else if (doYZ) {
            cv += cross_validation(vec_Y, test_matrices, vecH, lambda_loop);
            cv += cross_validation(vec_Z, test_matrices, vecH, lambda_loop);
            cv /= 3.0;
        }

        long double gcv = general_cross_validation(vec_X, test_matrices, vecH, lambda_loop);


        if (doY) {
            gcv += general_cross_validation(vec_Y, test_matrices, vecH, lambda_loop);
            gcv /= 2.0;

        }  else if (doYZ) {
            gcv += general_cross_validation(vec_Y, test_matrices, vecH, lambda_loop);
            gcv += general_cross_validation(vec_Z, test_matrices, vecH, lambda_loop);
            gcv /= 3.0;
        }
        //const double rss = RSS(vec_X, test_matrices, vecH, lambda_loop);


        if (!std::isnan(gcv) && !std::isinf(cv)) {
            if (!GCV.empty() && gcv>GCV.back() && first_mini_gcv_idx == 0) {
                first_mini_gcv_idx = GCV.size()-1;
            }

            GCV.push_back(gcv);
            lambda_GCV.push_back(lambda_loop);
            sv.tab_GCV[lambda_loop] = gcv;
        }

        if (!std::isnan(cv) && !std::isinf(cv)) {
            if (!CV.empty() && cv>CV.back()&& first_mini_cv_idx == 0) {
                first_mini_cv_idx = CV.size()-1;
            }


            CV.push_back(cv);
            lambda_CV.push_back(lambda_loop);
            sv.tab_CV[lambda_loop] = cv;
        }

        /*if (!std::isnan(rss) && !std::isinf(rss)) {
            if (!RSS_vec.empty() && rss>RSS_vec.back() && first_mini_rss_idx == 0) {
                first_mini_rss_idx = RSS_vec.size()-1;
            }

            if (rss <= mini_rss + 1E-16 ) {
                mini_rss = rss;
                mini_rss_idx = RSS_vec.size();
            }
            RSS_vec.push_back(rss);
            RSS_lambda.push_back(lambda_loop);
            //qDebug()<<" cv="<<lambda_loop_exp<<CV.back();//<< has_positif;
        }*/
    }

    // A - Recherche de GCV --------------
    int i = 0; // Sauvegarde du tableau brute
    for (auto& v : sv.tab_GCV) {
        v.second = GCV[i++];
    }
    // Supprimer les premiers zéros qui correspondent à des lambdas très petits, ne permettant pas le calcul
    //      Trouver le premier élément non-zéro
    auto it = std::find_if(GCV.begin(), GCV.end(),
                           [](double val) { return val != 0.0; });

    //      Supprimer tous les zéros au début
    if (it != GCV.begin()) {
        GCV.erase(GCV.begin(), it);
    }
    auto nb_0_GCV = lambda_GCV.size() - GCV.size();
    // gaussian_filter
    GCV = gaussian_filter(GCV, 10.0, 1);
    //GCV = low_pass_filter(GCV, 0.1, 1);

    // Recherche de l'indice de la plus petite valeur
    auto min_gcv_ptr = std::min_element(GCV.begin(), GCV.end());

    auto lambda_min_gcv_idx = std::distance(GCV.begin(), min_gcv_ptr) + nb_0_GCV;

    // B - Recherche de CV --------------
    i = 0;
    for (auto& v : sv.tab_CV) {
        v.second = CV[i++];
    }
    auto it2 = std::find_if(CV.begin(), CV.end(),
                           [](double val) { return val != 0.0; });

    // Supprimer tous les zéros au début
    if (it2 != CV.begin()) {
        CV.erase(CV.begin(), it2);
    }
    auto nb_0_CV = lambda_CV.size() - CV.size();
    CV = gaussian_filter(CV, 10.0, 1);

    // Recherche de l'indice de la plus petite valeur
    auto min_cv_ptr = std::min_element(CV.begin(), CV.end());

    auto lambda_min_cv_idx = std::distance(CV.begin(), min_cv_ptr) + nb_0_CV;

    qDebug()<<"[initLambdaSplineBySilverman] min_gcv_idx"<< lambda_GCV[lambda_min_gcv_idx] << lambda_CV[lambda_min_cv_idx];

    // If the mini is at one of the bounds, there is no solution in the interval for GCV
    // See if there is a solution in CV
    double lambda_mini;

    if (lambda_min_gcv_idx > 0 && lambda_min_gcv_idx <static_cast<size_t>(CV.size()-1)) { // solution évidente avec GCV
        lambda_mini = lambda_GCV[lambda_min_gcv_idx];
        sv.comment = "GCV solution; ";

    } else  if (lambda_min_cv_idx > 0 && lambda_min_cv_idx < static_cast<size_t>(CV.size()-1)) { // solution évidente avec CV
        lambda_mini = lambda_CV[lambda_min_cv_idx];
        sv.comment = "CV solution; ";

    } else {

        if (GCV.front() > GCV.back()) {
            lambda_mini = 10.0E20;
           // mini_gcv_idx = (GCV.size()-1);
            sv.comment = "No GCV solution; Linear Regression; ";

        } else {
            lambda_mini = 0.0; //lambda_GCV.front();
            //mini_gcv_idx = 0;
            sv.comment = "No GCV solution; Spline Interpolation; ";
        }

    }
    // test positif

/*    if (sv.force_positive_curve) {

        bool has_positif = false;

        int idx_depth = 0;
        double lambda_depth, Vg_depth;
        // Recomposition de vec_theta_red
        // Attention nous avons perdu la position du premier temps, il est considèré égale à Zéro pour le test
        std::vector<double> vec_theta_red;
        double som = 0.0;
        vec_theta_red.push_back(som);
        for (auto& h : vecH) {
            som += h;
            vec_theta_red.push_back(som);
        }

        do {
            lambda_depth =  lambda_mini * pow(10, (double)idx_depth/10.0);


            const SplineMatrices &spline_matrices = prepare_calcul_spline(vecH, W_1);

            //  Spline final

            const Matrix2D &tmp = multiConstParMat(spline_matrices.matQTW_1Q, lambda_depth, 5);
            const Matrix2D &matB = addMatEtMat(spline_matrices.matR, tmp, 5);


            // Decomposition_Cholesky de matB en matL et matD
            // Si alpha global: calcul de Mat_B = R + alpha * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
            const std::pair<Matrix2D, MatrixDiag> &decomp = decompositionCholesky(matB, 5, 1);

            const SplineResults &sx = do_spline(vec_X, spline_matrices,  vecH, decomp, lambda_depth);

            const std::vector<double> &vecVarG = calcul_spline_variance(spline_matrices, decomp, lambda_depth);


            MCMCSplineComposante splineX;
            splineX.vecThetaReduced = vec_theta_red;
            splineX.vecG = std::move(sx.vecG);
            splineX.vecGamma = std::move(sx.vecGamma);

            splineX.vecVarG = vecVarG;

            MCMCSpline spline;
            spline.splineX = std::move(splineX);

            has_positif =  hasPositiveGPrimeByDet(spline.splineX);//hasPositiveGPrimePlusConst ((spline.splineX, 0.);// hasPositiveGPrimeByDet(spline.splineX);
            if (!has_positif) {
                idx_depth++;
            }

        } while (!has_positif && lambda_depth<1E10);


        Vg_depth = var_residual(vec_X, test_matrices, vecH, lambda_depth);

        if (doY) {
            Vg_depth += var_residual(vec_Y, test_matrices, vecH, lambda_depth);
            Vg_depth /= 2.;

        } else if (doYZ) {
            Vg_depth += var_residual(vec_Y, test_matrices, vecH, lambda_depth) + var_residual(vec_Z, test_matrices, vecH, lambda_depth);
            Vg_depth /= 3.;
        }
#if DEBUG
        if (std::isnan(Vg_depth))
            qDebug()<<" Vg is nan in Silverman";
#endif
        //------
        return std::make_pair(lambda_depth, Vg_depth);

    } else {
*/
        double Vg = var_residual(vec_X, test_matrices, vecH, lambda_mini);

        if (doY) {
            Vg += var_residual(vec_Y, test_matrices, vecH, lambda_mini);
            Vg /= 2.;

        } else if (doYZ) {
            Vg += var_residual(vec_Y, test_matrices, vecH, lambda_mini) + var_residual(vec_Z, test_matrices, vecH, lambda_mini);
            Vg /= 3.;
        }

        return std::make_pair(lambda_mini, Vg);
//    }

    // ----- RETURN
 //   return std::make_pair(1., 0.);

}

/**
 * @brief var_residual , compute the Residual Sum of Squares about the fitted curve.
 * see Green and Silverman
 * @param vec_Y
 * @param matrices
 * @param vecH
 * @param lambda
 * @return
 */
double var_residual(const std::vector<t_matrix>& vec_Y, const SplineMatrices& matrices, const std::vector<t_reduceTime>& vecH, const double lambda)
{
    std::unique_ptr<std::pair<Matrix2D, MatrixDiag>> decomp;

    // Decomposition_Cholesky de matB en matL et matD
    if (lambda != 0.0) {
        decomp = std::make_unique<std::pair<Matrix2D, MatrixDiag>>(decomp_matB(matrices, lambda));

    } else {
        decomp = std::make_unique<std::pair<Matrix2D, MatrixDiag>>(decompositionCholesky(matrices.matR, 5, 1));
    }

    //  Calcul de Mat_B = R + alpha * Qt * W-1 * Q

    const SplineResults& sy = do_spline(vec_Y, matrices,  vecH, *decomp, lambda);

    const std::vector<t_matrix>& matA = calculMatInfluence_origin(matrices, 1, *decomp, lambda);

    long double N = static_cast<long double>(vec_Y.size());

    long double res = 0.0L;
    const long double trace = compensated_sum(matA);

    for (size_t i = 0; i < vec_Y.size(); ++i) {
        long double residual = static_cast<long double>(sy.vecG[i] - vec_Y[i]);
        res += residual * residual  ;
    }
    long double EDF = N - trace;

    res /= EDF;

    /*long double W = 0.;
    for (auto& sy2 : matrices.diagWInv) {
        W += 1.0L / static_cast<long double>(sy2);
    }


    long double long_res = 0.0L;
    const long double long_trace = compensated_sum(matA);

    for (size_t i = 0; i < vec_Y.size(); ++i) {
        long double residual = static_cast<long double>(sy.vecG[i] - vec_Y[i]);
        long double syi2 = static_cast<long double>(matrices.diagWInv[i]);
        long_res += residual * residual / syi2 ;
    }
    long double EDF = N - long_trace;

    long_res =  N / W / EDF * long_res;
    */
    return std::move(res);

}

std::vector<double> general_residual(const std::vector<t_matrix> &vec_Y,  const SplineMatrices &matrices, const std::vector<t_reduceTime> &vecH, const double lambda)
{
    const double N = matrices.diagWInv.size();

    const Matrix2D& matB = addMatEtMat(matrices.matR, multiConstParMat(matrices.matQTW_1Q, lambda, 5), 5);

    // Decomposition_Cholesky de matB en matL et matD
    // Si alpha global: calcul de Mat_B = R + lambda * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
    //std::pair<Matrix2D, std::vector<double>> decomp = decompositionCholesky(matB, 5, 1);
    //SplineResults s = calculSplineX (matrices, vecH, decomp, matB, lambdaSpline);

    //auto matA = calculMatInfluence_origin(matrices, s , 1, lambdaSpline);
    const auto& decomp = decompositionCholesky(matB, 5, 1);

    const SplineResults &s = do_spline(vec_Y, matrices,  vecH, decomp, lambda);
    const std::vector<t_matrix>& matA = calculMatInfluence_origin(matrices, 1, decomp, lambda);

    // Nombre de degré de liberté
    const auto DLEc = 1 - std::accumulate(matA.begin(), matA.end(), 0.0) / static_cast<t_matrix>(N);
    // 1 - calcul de sigma2 star
    /*double sum_w = 0.;
    for (auto& w_1 : matrices.diagWInv) {
        sum_w += 1./w_1;
    }
    sum_w /= (double)matrices.diagWInv.size() ;
    */
   /* double sig2 = var_residual(vec_Y, matrices, vecH, lambda)*sum_w;
    for (int i = 0 ; i < N; i++) {
        sig2 +=  pow( vec_Y[i] - s.vecG.at(i), 2) /matrices.diagWInv.at(i);
    }
   */

    // 2- calcul des ri star

    double sigma = 1.0;//sqrt(var);
    std::vector<double> g_res;
    for (int i = 0 ; i < N; i++) {
        g_res.push_back(  ( vec_Y[i] - s.vecG.at(i))/ (sigma*sqrt(matrices.diagWInv.at(i))*sqrt(DLEc)) );
    }

    return g_res;
}

bool  hasPositiveGPrimeByDet (const MCMCSplineComposante &splineComposante)
{

    for (unsigned long i= 0; i< splineComposante.vecThetaReduced.size()-1; i++) {

        const double t_i = splineComposante.vecThetaReduced.at(i);
        const double t_i1 = splineComposante.vecThetaReduced.at(i+1);
        const double hi = t_i1 - t_i;

        const double gamma_i = splineComposante.vecGamma.at(i);
        const double gamma_i1 = splineComposante.vecGamma.at(i+1);

        const double g_i = splineComposante.vecG.at(i);
        const double g_i1 = splineComposante.vecG.at(i+1);

        const double a = (g_i1 - g_i) /hi;
        const double b = (gamma_i1 - gamma_i) /(6*hi);
        const double s = t_i + t_i1;
        const double p = t_i * t_i1;
        const double d = ( (t_i1 - 2*t_i)*gamma_i1 + (2*t_i1 - t_i)*gamma_i )/(6*hi);
        // résolution équation

        const double aDelta = 3* b;
        const double bDelta = 2*d - 2*s*b;
        const double cDelta = p*b - s*d + a;

        const double delta = pow(bDelta, 2.) - 4*aDelta*cDelta;
        if (delta < 0) {
            if (aDelta < 0) // convexe
                return false;
            else           // concave
                continue;
        }

        double t1_res = (-bDelta - sqrt(delta)) / (2*aDelta);
        double t2_res = (-bDelta + sqrt(delta)) / (2*aDelta);

        if (t1_res > t2_res)
            std::swap(t1_res, t2_res);


        if (aDelta > 0) { //C'est un maximum entre les solutions
            if (!( t_i1 < t1_res || t2_res< t_i)) {
                return false;
            }

        } else { //C'est un minimum entre les solutions
            if ( !( t1_res < t_i && t_i1 < t2_res) )
                return false;
        }


        /*
        if (a < 0) {
            return false;

        } else if ( t_i < t1_res && t1_res< t_i1) {
            return false;

        } else if ( t_i < t2_res && t2_res< t_i1) {
            return false;
        }
*/
    }

    return true;
}

void spread_theta_reduced(std::vector<t_reduceTime> &sorted_t_red, t_reduceTime spread_span)
{
    std::vector<t_reduceTime>::iterator it_first = sorted_t_red.end();
    std::vector<t_reduceTime>::iterator it_last = sorted_t_red.end();
    unsigned nbEgal = 0;

    if (spread_span == 0.) {
        spread_span = 1.E-8; //std::numeric_limits<double>::epsilon() * 1.E12;//1.E6;// epsilon = 1E-16
    }

    // repère première egalité
    for (std::vector<t_reduceTime>::iterator it = sorted_t_red.begin(); it != sorted_t_red.end() -1; it++) {

        if (*std::next(it) - *it <= spread_span) {

            if (it_first == sorted_t_red.end()) {
                it_first = it;
                it_last = it + 1;
                nbEgal = 2;

            } else {
                it_last = it + 1;
                ++nbEgal;
            }

        } else {
            if (it_first != sorted_t_red.end()) {
                // on sort d'une égalité, il faut répartir les dates entre les bornes
                // itEvent == itEventLast
                const t_reduceTime lowBound = it_first == sorted_t_red.begin() ? sorted_t_red.front() : *std::prev(it_first) ; //valeur à gauche non égale
                const t_reduceTime upBound = it == sorted_t_red.end()-2 ? sorted_t_red.back(): *std::next(it);

                t_reduceTime step = spread_span / (nbEgal-1); // écart théorique
                t_reduceTime min;

                // Controle du debordement sur les valeurs encadrantes
                if (it_first == sorted_t_red.begin()) {
                   // Cas de l'égalité avec la première valeur de la liste
                   // Donc tous les Events sont à droite de la première valeur de la liste
                   min = *it;

                } else {
                   // On essaie de placer une moitier des Events à gauche et l'autre moitier à droite
                   min = *it- step*floor(nbEgal/2.);
                   // controle du debordement sur les valeurs encadrantes
                   min = std::max(lowBound + step, min );
                }

                const double max = std::min(upBound - spread_span, *it + (double)(step*ceil(nbEgal/2.)) );
                step = (max- min)/ (nbEgal - 1); // écart corrigé

                std::vector<t_reduceTime>::iterator it_egal;
                int count;
                for (it_egal = it_first, count = 0; it_egal != it+1; it_egal++, count++ ) {
                   *it_egal = min + count*step;
                }
                // Fin correction, prêt pour nouveau groupe/cravate
                it_first = sorted_t_red.end();

            }
        }


    }

    // sortie de la boucle avec itFirst validé donc itEventLast == sortedEvents.end()-1

    if (it_first != sorted_t_red.end()) {
        // On sort de la boucle et d'une égalité, il faut répartir les dates entre les bornes
        // itEvent == itEventLast
        const t_matrix lowBound = *std::prev(it_first); //la première valeur à gauche non égale

        const t_matrix max = sorted_t_red.back();
        t_matrix step = spread_span / (nbEgal-1.); // ecart théorique

        const t_matrix min = std::max(lowBound + spread_span, max - step*(nbEgal-1) );

        step = (max- min)/ (nbEgal-1); // écart corrigé

        // Tout est réparti à gauche
        int count;
        std::vector<t_reduceTime>::iterator it_egal;
        for (it_egal = it_first, count = 0; it_egal != sorted_t_red.end(); it_egal++, count++ ) {
            *it_egal = min + count *step;
        }

    }

}

/**
 * @brief get_order returns the indices that order the data in the vector
 * @param vec
 * @return
 */
std::vector<int> get_order(const std::vector<t_reduceTime> &vec)
{
    struct vec_index {
        vec_index(double _t, int _idx) {
            t = _t;
            idx = _idx;
        }
        double t;
        int idx;
    };
    std::vector<vec_index> tmp;
    int i = 0;
    for (auto t : vec)
        tmp.push_back(vec_index(t, i++));

    std::sort(tmp.begin(), tmp.end(), [](const vec_index a, const vec_index b) { return (a.t < b.t); });

    std::vector<int> res;
    for (auto t_i : tmp)
        res.push_back(t_i.idx);

    return res;
}

std::pair<Matrix2D, MatrixDiag> decomp_matB (const SplineMatrices& matrices, const double lambdaSpline)
{
    Q_ASSERT(lambdaSpline != 0);
    /*
    * if lambdaSpline == 0, it is interpolation
    * return decompositionCholesky(matrices.matR, 5, 1);
    */

    // Decomposition_Cholesky de matB en matL et matD
    // Si lambda global: calcul de Mat_B = R + lambda * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
    const Matrix2D &tmp = multiConstParMat(matrices.matQTW_1Q, lambdaSpline, 5);

    const Matrix2D &matB = addMatEtMat(matrices.matR, tmp, 5);
    return decompositionCholesky(matB, 5, 1);

}
