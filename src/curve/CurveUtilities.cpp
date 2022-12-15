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

#include "CurveUtilities.h"

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
    const unsigned n = rVecH.size()+1;

    // matQ est de dimension n x n-2, mais contenue dans une matrice nxn
    Matrix2D matQ = initMatrix2D(n, n);
    // On parcourt n-2 valeurs :
    for (unsigned i = 1; i < n-1; ++i) {
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
    stream << (quint32) splineComposante.vecThetaEvents.size();
    for (auto& v : splineComposante.vecThetaEvents)
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

    splineComposante.vecThetaEvents.resize(siz);
    std::generate_n(splineComposante.vecThetaEvents.begin(), siz, [&stream, &v]{stream >> v; return v;});

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

