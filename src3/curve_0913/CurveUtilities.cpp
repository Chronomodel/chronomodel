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
/*
std::vector<double> calculVecH(const std::vector<double>& vec)
{
    std::vector<double> result;
    for (unsigned long i = 0; i < (vec.size() - 1); ++i) {
        result.push_back(vec[i + 1] - vec[i]);
    }
    return result;
}*/

std::vector<double> calculVecH(const std::vector<double>& vec)
{
    // 2 codes possible
    /*
    std::vector<double> result(vec.size() - 1);
    std::transform(vec.begin(), vec.end()-1, vec.begin()+1,  result.begin(),  [](int v, int v1) {return v1-v; } );
    */

    std::vector<double>result(vec.size());
    std::adjacent_difference (vec.begin(), vec.end(), result.begin());
    result.erase(result.begin());

    return result;
}

// --------------- Function with list of double value

Matrix2D calculMatR(const std::vector<double>& vec)
{
    // Calcul de la matrice R, de dimension (n-2) x (n-2) contenue dans une matrice n x n
    // Par exemple pour n = 5 :
    // 0 0 0 0 0
    // 0 X X X 0
    // 0 X X X 0
    // 0 X X X 0
    // 0 0 0 0 0

    // vecH est de dimension n-1
    std::vector<double> vecH = calculVecH(vec);
    const unsigned n = vec.size();

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
    for ( unsigned i = 1; i < n-2; ++i) {
        matR[i][i] = (vecH[i-1] + vecH[i]) / 3.;
        matR[i][i+1] = vecH[i] / 6.;
        matR[i+1][i] = vecH[i] / 6.;
    }
    // Si on est en n-2 (dernière itération), on ne calcule pas les valeurs de part et d'autre de la diagonale (termes symétriques)
   matR[n-2][n-2] = (vecH[n-2-1] + vecH[n-2]) / 3.;

    return matR;
}

Matrix2D calculMatQ(const std::vector<double>& vec)
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
    std::vector<double> vecH = calculVecH(vec);
    const unsigned n = vec.size();

    // matQ est de dimension n x n-2, mais contenue dans une matrice nxn
    Matrix2D matQ = initMatrix2D(n, n);
    // On parcourt n-2 valeurs :
   /* for (unsigned i = 1; i < vecH.size(); ++i) {
        matQ[i-1][i] = 1. / vecH[i-1];
        matQ[i][i] = -((1./vecH[i-1]) + (1./vecH[i]));
        matQ[i+1][i] = 1. / vecH[i];
    }*/

    for (unsigned i = 1; i < vecH.size(); ++i) {
            matQ[i-1][i] = 1. / vecH[i-1];
            matQ[i+1][i] = 1. / vecH[i];
            matQ[i][i] = -(matQ[i-1][i] + matQ[i+1][i]);
        }


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
std::vector<double> CurveUtilities::definitionNoeuds(const std::vector<double> &tabPts, const double minStep)
{
   // display(tabPts);

    std::vector<double> result (tabPts);
    std::sort(result.begin(), result.end());
    
    // Espacement possible ?
    if ((result[result.size() - 1] - result.at(0)) < (result.size() - 1) * minStep) {
        qCritical("Pas assez de place pour écarter les points");
        exit(0);
    }
    
    //display(result);
    
    // Il faut au moins 3 points
    if (result.size() >= 3) {
        // 0 veut dire qu'on n'a pas détecté d'égalité :
        std::size_t startIndex = 0;
        std::size_t endIndex = 0;
        double value ;
        double lastValue;
        for (std::size_t i = 1; i<result.size(); ++i) {
            value = result.at(i);
            lastValue = result.at(i - 1);
            
            // Si l'écart n'est pas suffisant entre la valeur courante et la précedente,
            // alors on mémorise l'index précédent comme le début d'une égalité
            // (à condition de ne pas être déjà dans une égalité)
            if ((value - lastValue < minStep) && (startIndex == 0)) {
                // La valeur à l'index 0 ne pourra pas être déplacée vers la gauche !
                // S'il y a égalité dès le départ, on considère qu'elle commence à l'index 1.
                startIndex = (i == 1) ? 1 : (i-1);
            }
            
            //std::cout << "i = " << i << " | value = " << value << " | lastValue = " << lastValue << " | startIndex = " << startIndex << std::endl;
            
            // Si on est à la fin du tableau et dans un cas d'égalité,
            // alors on s'assure d'avoir suffisamment d'espace disponible
            // en incluant autant de points précédents que nécessaire dans l'égalité.
            if ((i == result.size() - 1) && (startIndex != 0)) {
                endIndex = i-1;
                for (std::size_t j = startIndex; j >= 1; j--) {
                    const double delta = value - result.at(j-1);
                    const double deltaMin = minStep * (i - j + 1);

                    if (delta >= deltaMin) {
                        startIndex = j;
                        qWarning("=> Egalité finale | startIndex = %d | endIndex = %d" ,(int)startIndex , (int)endIndex );
                        break;
                    }
                }
            }
            
            // Si l'écart entre la valeur courante et la précédente est suffisant
            // ET que l'on était dans un cas d'égalité (pour les valeurs précédentes),
            // alors il se peut qu'on ait la place de les espacer.
            if ((value - lastValue >= minStep) && (startIndex != 0)) {
                const double startValue = result.at(startIndex-1);
                const double delta = (value - startValue);
                const double deltaMin = minStep * (i - startIndex + 1);
                
                qWarning("=> Vérification de l'espace disponible | delta = %f  | deltaMin = %f ",delta ,deltaMin);
                
                if (delta >= deltaMin) {
                    endIndex = i-1;
                }
            }
            
            if (endIndex != 0) {
                qWarning( "=> On espace les valeurs entre les bornes %f et %f", result[startIndex - 1], result[i]);
                
                // On a la place d'espacer les valeurs !
                // - La borne inférieure ne peut pas bouger (en startIndex-1)
                // - La borne supérieure ne peut pas bouger (en endIndex)
                // => On espace les valeurs intermédiaires (de startIndex à endIndex-1) du minimum nécessaire
                const double startSpread = result.at(endIndex) - result.at(startIndex);
                for (std::size_t j = startIndex; j <= endIndex; j++) {
                    // !!! pHd : Ici on risque de décaller toutes les valeurs vers la droite,
                    // on peut finir par avoir la dernière décallée de (endIndex-startIndex)*minStep
                    if (result.at(j) - result.at(j-1) < minStep) {
                        result[j] = result.at(j-1) + minStep;
                    }
                }


                // En espaçant les valeurs vers la droite, on a "décentré" l'égalité.
                // => On redécale tous les points de l'égalité vers la gauche pour les recentrer :
                double endSpread = result.at(endIndex) - result.at(startIndex);
                double shiftBack = (endSpread - startSpread) / 2;
                
                // => On doit prendre garde à ne pas trop se rappocher le la borne de gauche :
                if ((result.at(startIndex)  - shiftBack) - result.at(startIndex-1) < minStep) {
                    shiftBack = result.at(startIndex) - (result.at(startIndex-1) + minStep);
                }
                
                // On doit décaler suffisamment vers la gauche pour ne pas être trop près de la borne de droite :
                if (result.at(endIndex + 1) - (result.at(endIndex) - shiftBack) < minStep) {
                    shiftBack = result.at(endIndex) - (result.at(endIndex + 1) - minStep);
                }
                /*for (unsigned long j=startIndex; j<=endIndex; j++) {
                    result[j] -= shiftBack;
                }*/
                std::transform(result.begin() + startIndex, result.begin() + endIndex, result.begin() + startIndex, [&shiftBack](double v){return v-shiftBack;});
                // On marque la fin de l'égalité
                startIndex = 0;
                endIndex = 0;
            }
        }
    }
    
    return result;
    
    /*
    bool hasEquality = false;
    bool equalityEnded = false;
    vector<double>::iterator equalityStartIterator;
    vector<double>::iterator equalityEndIterator;
    double equalityStartValue;
    double equalityEndValue;
    
    for (vector<double>::iterator it=result.begin(); it!=result.end(); ++it)
    {
        double val = *it;
        double nextVal = NULL;
        
        ++it;
        if(it != result.end()){
            nextVal = *it;
            if(nextVal - val < minStep){
                // On a une égalité
                if(!hasEquality == true){
                    // C'est le début d'une nouvelle égalité
                    hasEquality = true;
                    // On se souvient de la borne inférieure
                    --it;
                    equalityStartIterator = it;
                    if(it != result.begin()){
                        --it;
                        equalityStartValue = *it;
                        ++it;
                    }else{
                        equalityStartValue = *it;
                    }
                    ++it;
                }else{
                    // On avait déjà une égalité. On attend juste d'en trouver la fin
                }
            }else{
                // On a pas égalité
                if(hasEquality == true){
                    // On a atteint la fin de l'égalite
                    // TODO : On vérifie si on a la place d'écarter les valeurs
                    // Si oui, alors on écarte les pts lors de cette itération
                    // Si non, on fait rentrer la valeur courante dans l'égalité et on continue à parcourir les pts jusqu'à avoir l'espace suffisant pour écarter les points en égalité
                    equalityEnded = true;
                    equalityEndIterator = it;
                    equalityEndValue = *it;
                }
            }
        }else{
            if(hasEquality == true){
                // On a atteint la fin sur une égalite : on garde la dernière borne, puis on espace les éléments
                equalityEnded = true;
                equalityEndIterator = it-1;
                equalityEndValue = *(it-1);
            }
        }
        --it;
        
        // Do the shift if necessary
        if(equalityEnded){
            cout << ' ' << *equalityStartIterator << ' ' << *equalityEndIterator;
            
            ResolT = ---
            
            t0 ---------- t1 --- t2 - t3 - t4 -- t5
            
            t0 ----- t1 --- t2 --- t3 --- t4 --- t5
            
            double start; // valeur de t1
            double end; // valeur de t6
            
            
            deltaCravate = t5 - t2;
            deltaDispo = t6 - t1;
            deltaTotal = 3 * minStep;
            deltaCumul = 0;
            
            for(t = t2 à t5)
            {
                
                
                t'2 = t2 - 3 * minStep / 2;
                t'3 = t2 - 1 * minStep / 2;
                t'4 = t2 + 1 * minStep / 2;
                t'5 = t2 + 3 * minStep / 2;
                
                deltaT = (1 + random()) * minStep;
                
                t = deltaCumul + deltaT;
                deltaCumul += deltaT;
                
                
                
                
                delta2 = minStep * (1 + random());
                t2 += delta2;
                deltaTotal += delta2;
            }
            
            
            hasEquality = false;
            equalityEnded = false;
        }
    }
    
    for (vector<double>::iterator it=result.begin(); it!=result.end(); ++it)
        cout << ' ' << *it;
    */
    return result;
}


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

