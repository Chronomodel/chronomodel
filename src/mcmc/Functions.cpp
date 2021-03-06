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

#include "Functions.h"
#include "Generator.h"
#include "QtUtilities.h"

#include <QDebug>
#include <QApplication>
#include <set>
#include <map>
#include <QTime>

// -----------------------------------------------------------------
//  sumP = Sum (pi)
//  sum = Sum (pi * xi)
//  sum2 = Sum (pi * xi^2)
// -----------------------------------------------------------------

/**
 * @brief Product a FunctionAnalysis from a QMap
 * @todo Handle empty function case and null density case (pi = 0)
*/
FunctionAnalysis analyseFunction(const QMap<type_data, type_data> &aFunction)
{
    FunctionAnalysis result;
    if (aFunction.isEmpty()) {
        result.max = (type_data)0.;
        result.mode = (type_data)0.;
        result.mean = (type_data)0.;
        result.stddev = (type_data)(-1.);
        qDebug() << "WARNING : in analyseFunction() aFunction isEmpty !! ";
        return result;
    }

    type_data max (0.);
    type_data mode (0.);
    type_data sum (0.);
    type_data sum2 (0.);
    type_data sumP (0.);

    type_data prevY (0.);
    QList<type_data> uniformXValues;

    QMap<type_data,type_data>::const_iterator citer = aFunction.cbegin();
    for (;citer != aFunction.cend(); ++citer) {
        const type_data x = citer.key();
        const type_data y = citer.value();

        sumP += y;
        sum += y * x;
        sum2 += y * x * x;

        if (max <= y) {
            max = y;
            if (prevY == y) {
                uniformXValues.append(x);
                int middleIndex = floor(uniformXValues.size()/2);
                mode = uniformXValues.at(middleIndex);
            } else {
                uniformXValues.clear();
                mode = x;
            }
        }
        prevY = y;
    }

    //FunctionAnalysis result;
    result.max = max;
    result.mode = mode;
    result.mean = (type_data)0.;
    result.stddev = (type_data)0.;

    if (sumP != 0) {
        result.mean = sum / sumP;
        type_data variance = (sum2 / sumP) - pow(result.mean, 2);

        if (variance < 0) {
            qDebug() << "WARNING : in analyseFunction() negative variance found : " << variance<<" return 0";
            variance = -variance;
        }

        result.stddev = sqrt(variance);
    }

    return result;
}


type_data dataStd(const QVector<type_data> &data)
{
    // Work with double precision here because sum2 might be big !

    const type_data s = sum<type_data>(data);
    const type_data s2 = sum2<type_data>(data);
    const type_data mean = s / data.size();
    const type_data variance = s2 / data.size() - mean * mean;

    if (variance < 0) {
        qDebug() << "WARNING : in dataStd() negative variance found : " << variance<<" return 0";
        return (type_data)0.;
    }
    return sqrt(variance);
}

double shrinkageUniform(const double so2)
{
    //double u = Generator::randomUniform();
    const double u = Generator::randomUniform(0,1);
    return (so2 * (1. - u) / u);
}

/**
 * @brief Return a text from a FunctionAnalysis
 * @see FunctionAnalysis
 */
QString functionAnalysisToString(const FunctionAnalysis& analysis, const bool forCSV)
{
    QString result;

    if (analysis.stddev<0.)
        result = QObject::tr("No data");

    else
        if (forCSV) {
            result += QObject::tr("MAP = %1  ;  Mean = %2  ;  Std deviation = %3").arg(stringForCSV(analysis.mode),
                                                                                    stringForCSV(analysis.mean),
                                                                                    stringForCSV(analysis.stddev));
        }  else {
            result += QObject::tr("MAP = %1  ;  Mean = %2  ;  Std deviation = %3").arg(stringForLocal(analysis.mode),
                                                                                    stringForLocal(analysis.mean),
                                                                                    stringForLocal(analysis.stddev));
        }


    return result;
}

/**
 * @brief Return a text with the value of th Quartiles Q1, Q2 and Q3
 * @see DensityAnalysis
 */
QString densityAnalysisToString(const DensityAnalysis& analysis, const QString& nl, const bool forCSV)
{
    QString result (QObject::tr("No data"));
    if (analysis.analysis.stddev>=0.) {
        result = functionAnalysisToString(analysis.analysis, forCSV) + nl;
        if (forCSV){
            result += QObject::tr("Q1 = %1  ;  Q2 (Median) = %2  ;  Q3 = %3 ").arg(stringForCSV(analysis.quartiles.Q1),
                                                                                stringForCSV(analysis.quartiles.Q2),
                                                                                stringForCSV(analysis.quartiles.Q3));
        } else {
            result += QObject::tr("Q1 = %1  ;  Q2 (Median) = %2  ;  Q3 = %3 ").arg(stringForLocal(analysis.quartiles.Q1),
                                                                                stringForLocal(analysis.quartiles.Q2),
                                                                                stringForLocal(analysis.quartiles.Q3));
        }

    }
    return result;
}


Quartiles quartilesForTrace(const QVector<type_data> &trace)
{
    Quartiles quartiles = quartilesType(trace, 8, 0.25);
    return quartiles;

    /* Old function
    Quartiles quartiles;
    const int n = trace.size();
    if (n<5) {
        quartiles.Q1 = 0.;
        quartiles.Q2 = 0.;
        quartiles.Q3 = 0.;
        return quartiles;
    }

    QVector<type_data> sorted (trace);
    std::sort(sorted.begin(),sorted.end());

    const int q1index = (int) ceil(n * 0.25);
    const int q3index = (int) ceil(n * 0.75);

    quartiles.Q1 = sorted.at(q1index);
    quartiles.Q3 = sorted.at(q3index);

    if (n % 2 == 0) {
        const int q2indexLow = n / 2;
        const int q2indexUp = q2indexLow + 1;

        quartiles.Q2 = sorted.at(q2indexLow) + (sorted.at(q2indexUp) - sorted.at(q2indexLow)) / 2.;
    } else {
        const int q2index = (int)ceil(n * 0.5);
        quartiles.Q2 = sorted.at(q2index);
    }
    return quartiles;
    */
}

QVector<double> calculRepartition (const QVector<double>& calib)
{
    QVector<double> repartitionTemp;

    // we use long double type because
    // after several sums, the repartion can be in the double type range
    long double lastRepVal (0.);
    for (auto &&v : calib) {
        long double lastV = v;

        long double rep = lastRepVal;
        if(v != 0. && lastV != 0.)
            rep = lastRepVal + (lastV + v) / 2.;

        repartitionTemp.append((double)rep);
        lastRepVal = rep;
    }
    return repartitionTemp;
}

QVector<double> calculRepartition (const QMap<double, double>  &calib)
{
    QVector<double> repartitionTemp;

    // we use long double type because
    // after several sums, the repartion can be in the double type range
    long double lastV = calib.value(calib.firstKey());
    double lastT = calib.firstKey();
    QMap<double, double>::const_iterator it (calib.cbegin());
    long double lastRepVal (0.);

    while (it != calib.cend()) {
        double v = it.value();
        double t = it.key();
        long double rep = lastRepVal;
        if(v != 0. && lastV != 0.)
            rep = lastRepVal + (t-lastT)*(lastV + v) / 2.;

        lastV = v;
        lastT = t;

        repartitionTemp.append((double)rep);
        lastRepVal = rep;
        ++it;
    }
    // normalize repartition
    for (auto &&v : repartitionTemp)
        v = v/lastRepVal;

    return repartitionTemp;
}
Quartiles quartilesForRepartition(const QVector<double>& repartition, const double tmin, const double step)
{
    Quartiles quartiles;
    if (repartition.size()<5) {
        quartiles.Q1 = 0.;
        quartiles.Q2 = 0.;
        quartiles.Q3 = 0.;
        return quartiles;
    }
    const double q1index = vector_interpolate_idx_for_value(0.25, repartition);
    const double q2index = vector_interpolate_idx_for_value(0.5, repartition);
    const double q3index = vector_interpolate_idx_for_value(0.75, repartition);

    quartiles.Q1 = tmin + q1index * step;
    quartiles.Q2 = tmin + q2index * step;
    quartiles.Q3 = tmin + q3index * step;

    return quartiles;
}
/**
 * @brief credibilityForTrace find the smallest interval Credibility with the confidence thresh
 * @param trace
 * @param thresh
 * @param exactThresholdResult
 * @param description
 * @return
 */
QPair<double, double> credibilityForTrace(const QVector<double>& trace, double thresh, double& exactThresholdResult,const  QString description)
{
    (void) description;
    QPair<double, double> credibility(0.,0.);
    exactThresholdResult = 0.;
    const int n = trace.size();
    if (thresh > 0 && n > 0) {
        double threshold = inRange(0.0, thresh, 100.0);
        QVector<double> sorted (trace);
        std::sort(sorted.begin(),sorted.end());

        const int numToRemove = (int)floor(n * (1. - threshold / 100.));
        exactThresholdResult = ((double)n - (double)numToRemove) / (double)n;

        double lmin = 0.;
        int foundJ = 0;

        for (int j=0; j<=numToRemove; ++j) {
            const double l = sorted.at((n - 1) - numToRemove + j) - sorted.at(j);
            if ((lmin == 0.f) || (l < lmin)) {
                foundJ = j;
                lmin = l;
            }
        }
        credibility.first = sorted.at(foundJ);
        credibility.second = sorted.at((n - 1) - numToRemove + foundJ);
    }

    if (credibility.first == credibility.second) {
        //It means : there is only one value
        return QPair<double, double>();
    }
    else return credibility;
}

// Used in generateTempo for credibility
QPair<double, double> credibilityForTrace(const QVector<int>& trace, double thresh, double& exactThresholdResult,const  QString description)
{
    (void) description;
    QPair<double, double> credibility(0.,0.);
    exactThresholdResult = 0.;
    const int n = trace.size();
    if (thresh > 0 && n > 0) {
        double threshold = inRange(0.0, thresh, 100.0);
        QVector<int> sorted (trace);
        std::sort(sorted.begin(),sorted.end());

        const int numToRemove = (int)floor(n * (1. - threshold / 100.));
        exactThresholdResult = ((double)n - (double)numToRemove) / (double)n;

        double lmin (0.);
        int foundJ (0);

        for (int j=0; j<=numToRemove; ++j) {
            const double l = sorted.at((n - 1) - numToRemove + j) - sorted.at(j);
            if ((lmin == 0.) || (l < lmin)) {
                foundJ = j;
                lmin = l;
            }
        }
        credibility.first = (double)sorted.at(foundJ);
        credibility.second = (double)sorted.at((n - 1) - numToRemove + foundJ);
    }

    if (credibility.first == credibility.second) {
        //It means : there is only one value
        return QPair<double, double>();
    }
    else return credibility;
}


/**
 * @brief timeRangeFromTraces
 * @param trace1
 * @param trace2
 * @param thresh
 * @param description  compute type 7 R quantile
 * @return
 */
QPair<double, double> timeRangeFromTraces(const QVector<double>& trace1, const QVector<double>& trace2, const double thresh, const QString description)
{
    (void) description;
    QPair<double, double> range(- INFINITY, +INFINITY);
#ifdef DEBUG
    QTime startTime (QTime::currentTime());
#endif
    // limit of precision, to accelerate the calculus
    const double epsilonStep = 0.1/100.;

    // if thresh is equal 0 then return an QPair=(-INFINITY,+INFINITY)

    const int n = trace1.size();

    if ( (thresh > 0) && (n > 0) && ((int)trace2.size() == n) ) {

        const double gamma = 1. - thresh/100.;

        double dMin = INFINITY;

        std::vector<double> traceAlpha (trace1.toStdVector());
        std::vector<double> traceBeta (trace2.size());

        // 1 - map with relation Beta to Alpha
        std::multimap<double, double> betaAlpha;
        for(int i=0; i<trace1.size(); ++i)
            betaAlpha.insert(std::pair<double, double>(trace2.at(i),trace1.at(i)) );

        std::copy(trace2.begin(),trace2.end(),traceBeta.begin());

        // keep the beta trace in the same position of the Alpha, so we need to sort them with there values of alpha
        std::sort(traceBeta.begin(),traceBeta.end(),[&betaAlpha](const double i, const double j){ return betaAlpha.find(i)->second < betaAlpha.find(j)->second  ;} );

        std::sort(traceAlpha.begin(),traceAlpha.end());

        std::vector<double> betaUpper(n);

        // 2- loop on Epsilon to look for a and b with the smallest length
        for (double epsilon = 0.; epsilon <= gamma; ) {

            // original calcul according to the article const float ha( (traceAlpha.size()-1)*epsilon +1 );
            // We must decrease of 1 because the array begin at 0
            const double ha( (traceAlpha.size()-1)*epsilon);

            const int haInf ( floor(ha) );
            const int haSup ( ceil(ha) );

            const double a = traceAlpha.at(haInf) + ( (ha-haInf)*(traceAlpha.at(haSup)-traceAlpha.at(haInf)) );

            // 3 - copy only value of beta with alpha greater than a(epsilon)
            const int alphaIdx = ha==haInf ? haInf:haSup;

            const int remainingElemt =  n - alphaIdx;
            betaUpper.resize(remainingElemt);   // allocate space

            // traceBeta is sorted with the value alpha join
            auto it = std::copy( traceBeta.begin()+ alphaIdx, traceBeta.end(), betaUpper.begin() );

            const int betaUpperSize = (const int) std::distance(betaUpper.begin(), it);

            betaUpper.resize(betaUpperSize);  // shrink container to new size

            /*  std::nth_element has O(N) complexity,
             *  whereas std::sort has O(Nlog(N)).
             *  here we don't need complete sorting of the range, so it's advantageous to use it.
             *
             * std::nth_element(betaUpper.begin(), betaUpper.begin() + nTarget-1, betaUpper.end());
             *
             * in the future with C++17
             * std::experimental::parallel::nth_element(par,betaUpper.begin(), betaUpper.begin() + nTarget, betaUpper.end());
             *
             */

            // 4- We sort all the array
            std::sort(betaUpper.begin(), betaUpper.end());

           // 5 - Calcul b
            const double bEpsilon( (1.-gamma)/(1.-epsilon) );
            // original calcul according to the article const float hb( (betaUpper.size()-1)*bEpsilon +1 );
            // We must decrease of 1 because the array begin at 0
            const double hb ( (betaUpper.size() - 1.)*bEpsilon);
            const int hbInf ( floor(hb) );
            const int hbSup ( ceil(hb) );

            const double b = betaUpper.at(hbInf) + ( (hb-hbInf)*(betaUpper.at(hbSup)-betaUpper.at(hbInf)) );

            // 6 - keep the shortest length

            if ((b-a) < dMin) {
                dMin = b - a;
                range.first = a;
                range.second = b;
            }

            epsilon += epsilonStep;
        }
    }


#ifdef DEBUG
    qDebug()<<description;
    QTime timeDiff(0,0,0,1);
    timeDiff = timeDiff.addMSecs(startTime.elapsed()).addMSecs(-1);
    qDebug()<<"timeRangeFromTraces ->time elapsed = "<<timeDiff.hour()<<"h "<<QString::number(timeDiff.minute())<<"m "<<QString::number(timeDiff.second())<<"s "<<QString::number(timeDiff.msec())<<"ms" ;
#endif

    return range;
}

/**
 * @brief timeRangeFromTraces_old
 * @param trace1
 * @param trace2
 * @param thresh
 * @param description Obsolete function keep only for memory and test
 * @return
 */

QPair<double, double> transitionRangeFromTraces(const QVector<double>& trace1, const QVector<double>& trace2, const double thresh, const QString description)
{
    return timeRangeFromTraces(trace1, trace2, thresh, description);
}


/**
 * @brief gapRangeFromTraces find the gap between two traces, if there is no solution corresponding to the threshold, we return a QPair=(-INFINITY,+INFINITY)
 * @param traceBeta QVector of double corresponding to the first trace
 * @param traceAlpha QVector of double corresponding to the second trace
 * @param thresh Threshold to obtain
 * @param description a simple text
 * @return
 */
QPair<double, double> gapRangeFromTraces(const QVector<double>& traceEnd, const QVector<double>& traceBegin, const double thresh, const QString description)
{
    (void) description;
#ifdef DEBUG
    QTime startTime = QTime::currentTime();
#endif

    QPair<double, double> range = QPair<double, double>(- INFINITY, + INFINITY);

    // limit of precision, to accelerate the calculus, we set the same as RChronoModel
    const double epsilonStep = 0.1/100.;

    const int n = traceBegin.size();

    if ( (thresh > 0.f) && (n > 0) && ((int)traceEnd.size() == n) ) {

        const double gamma = 1. - thresh/100.;

        double dMax(0.);

        // We must change the type (float to double) to increase the precision
        std::vector<double> traceBeta (traceEnd.size());
        std::copy(traceEnd.begin(), traceEnd.end(), traceBeta.begin());

        std::vector<double> traceAlpha (traceBegin.size());
        std::copy(traceBegin.begin(),traceBegin.end(),traceAlpha.begin());

        // 1 - map with relation Alpha to Beta
        std::multimap<double, double> alphaBeta;
        for(int i=0; i<traceBegin.size(); ++i)
            alphaBeta.insert(std::pair<double, double>(traceAlpha.at(i),traceBeta.at(i)) );

        // keep the beta trace in the same position of the Alpha, so we need to sort them with there values of alpha
        std::sort(traceAlpha.begin(),traceAlpha.end(),[&alphaBeta](const double i, const double j){ return alphaBeta.find(i)->second < alphaBeta.find(j)->second  ;} );

        std::sort(traceBeta.begin(),traceBeta.end());

        std::vector<double> alphaUnder(n);

        // 2- loop on Epsilon to look for a and b with the smallest length
        // with a const epsilonStep increment, we not reach exactly gamma.
        // So we have to go hover gamma to find the value for exactly gamma

        for (double epsilon = 0.; epsilon <= gamma; ) {

            const double aEpsilon = 1. - epsilon;

            // Linear intertpolation according to R quantile( type=7)
            // We must decrease of 1 from the original formula because the array begin at 0
            const double ha( ((double)traceBeta.size()-1.) * aEpsilon);

            const int haInf( floor(ha) );
            const int haSup( ceil(ha) );

            if ((haSup > (int)traceBeta.size()) || (haInf > (int)traceBeta.size()))
                return range;
            if ((haInf < 0) || (haSup < 0))
                return range;

            const double a = traceBeta.at(haInf) + ( (ha-(double)haInf)*(traceBeta.at(haSup)-traceBeta.at(haInf)) );

            // 3 - copy only value of beta with alpha smaller than a(epsilon)!
            const int alphaIdx(haSup < n ? haSup : n-1 );//( ha == haInf ? haInf : haSup );//( ha == haSup ? haSup : haInf );// //

            const int remainingElemt ( alphaIdx );
            alphaUnder.resize(remainingElemt);   // allocate space

            // traceAlpha is sorted with the value alpha join
            auto it = std::copy( traceAlpha.begin(), traceAlpha.begin() + alphaIdx, alphaUnder.begin() );

            const int alphaUnderSize = (const int) std::distance(alphaUnder.begin(),it);

            alphaUnder.resize(alphaUnderSize);  // shrink container to new size

            // 4- We sort all the array
            std::sort(alphaUnder.begin(), alphaUnder.end());

           // 5 - Calcul b
            const double bEpsilon( (gamma-epsilon)/(1.-epsilon) );

            // Linear intertpolation like in R quantile( type=7)

            const double hb( ((double)alphaUnder.size()-1.)* bEpsilon );
            const int hbInf( floor(hb) );
            const int hbSup( ceil(hb) );

            if ((hbSup > (int)alphaUnder.size()) || (hbInf > (int)alphaUnder.size()))
                return range;
            if ((hbInf < 0) || (hbSup <0))
                return range;

            const double b = alphaUnder.at(hbInf) + ( (hb-(double)hbInf)*(alphaUnder.at(hbSup)-alphaUnder.at(hbInf)) );

            // 6 - keep the longest length

            if ((b-a) >= dMax) {
                dMax = b - a;
                range.first = a;
                range.second = b;
            }
            if (epsilon< gamma) {
                epsilon += epsilonStep;
                if (epsilon > gamma )
                    epsilon = gamma;

            // We detect that gamma is passed. To stop the loop we add one more step to exit
            } else if (epsilon >= gamma)
                epsilon += epsilonStep;
        }
    }

#ifdef DEBUG
    qDebug()<<description;
    QTime timeDiff(0,0,0,1);
    timeDiff = timeDiff.addMSecs(startTime.elapsed()).addMSecs(-1);
    qDebug()<<"gapRangeFromTraces ->time elapsed = "<<timeDiff.hour()<<"h "<<QString::number(timeDiff.minute())<<"m "<<QString::number(timeDiff.second())<<"s "<<QString::number(timeDiff.msec())<<"ms" ;
#endif

    return range;
}

/**
 * @brief gapRangeFromTraces_old
 * @param traceBeta
 * @param traceAlpha
 * @param thresh
 * @param description Obsolete function keep only for memory and test
 * @return
 */
QPair<float, float> gapRangeFromTraces_old(const QVector<float>& traceBeta, const QVector<float>& traceAlpha, const float thresh, const QString description)
{
    (void) description;
#ifdef DEBUG
    QTime startTime = QTime::currentTime();
#endif

    QPair<float, float> range = QPair<float, float>(- INFINITY, + INFINITY);
    // limit of precision, to accelerate the calculus
    const float perCentStep = 0.01f;

    const int n = traceBeta.size();
    if ( (thresh > 0) && (n > 0) && ((int)traceAlpha.size() == n) ) {

        const int nTarget = (const int) ceil( (float)n * thresh/100.);
        const int nGamma = n - nTarget;

        float dMax = 0.0;

        // make couple beta vs alpha in a std::map, it's a sorted container with ">"
        std::multimap<float,float> mapPair;

        QVector<float>::const_iterator ctB = traceBeta.cbegin();
        QVector<float>::const_iterator ctA = traceAlpha.cbegin();

        while (ctB != traceBeta.cend() ) {
            mapPair.insert(std::pair<float,float>(*ctB,*ctA));
            ++ctA;
            ++ctB;
        }

        // we suppose there is never the several time the same value inside traceBeta or inside traceAlpha
        // so we can just shift the iterator
        std::multimap<float,float>::const_reverse_iterator i_shift = mapPair.rbegin();

        std::vector<float> alphaUnder;
        alphaUnder.reserve(n);
        std::multimap<float,float>::const_reverse_iterator iMapTmp = i_shift;


        const int epsilonStep = qMax(1, (int)floor(nGamma*perCentStep));

        for (int nEpsilon = 0; (nEpsilon <= nGamma ) && (i_shift != mapPair.rend()); ) {

            // We use a reverse Iterator so the first is the last value in the QMap
            const float a = (*i_shift).first;//a=beta(i)=a(epsilon);

            // find position of beta egual a(epsilon)
            iMapTmp = i_shift;
            alphaUnder.clear();
            while (iMapTmp != mapPair.rend()) {
                alphaUnder.push_back((*iMapTmp).second);
                ++iMapTmp;
            }
            //std::sort(alphaUnder.begin(),alphaUnder.end());

            const int j = nGamma - nEpsilon;
            std::nth_element(alphaUnder.begin(), alphaUnder.begin() + j, alphaUnder.end());

            const float b = alphaUnder.at(j); //b=alpha(j)
            // keep the longest length
            if ((b-a) > dMax) {
                dMax = b-a;
                range.first = a;
                range.second = b;
            }

            nEpsilon = nEpsilon + epsilonStep;
            if (nEpsilon<=nGamma)
                std::advance(i_shift,epsilonStep);// reverse_iterator
        }

    }

#ifdef DEBUG
    qDebug()<<description;
    QTime timeDiff(0,0,0,1);
    timeDiff = timeDiff.addMSecs(startTime.elapsed()).addMSecs(-1);
    qDebug()<<"gapRangeFromTraces_old ->time elapsed = "<<timeDiff.hour()<<"h "<<QString::number(timeDiff.minute())<<"m "<<QString::number(timeDiff.second())<<"s "<<QString::number(timeDiff.msec())<<"ms" ;
#endif

    return range;
}



QString intervalText(const QPair<double, QPair<double, double> > &interval,  DateConversion conversionFunc, const bool forCSV)
{
    const double perCent = interval.first;
    const double inter1 = (conversionFunc ? conversionFunc(interval.second.first) : interval.second.first );
    const double inter2 = (conversionFunc ? conversionFunc(interval.second.second) : interval.second.second );

     if (forCSV)
         return "[ " + stringForCSV(inter1) + " ; " + stringForCSV(inter2) + " ] (" + stringForCSV(perCent) + "%)";
     else
         return "[ " + stringForLocal(interval.second.first) + " ; " + stringForLocal(interval.second.second) + " ] (" + stringForLocal(interval.first) + "%)";

/*
    if (forCSV) {
        if (formatFunc)
            return "[ " + formatFunc(interval.second.first, true) + " ; " + formatFunc(interval.second.second, true) + " ] (" + stringForCSV(interval.first) + "%)";

        else
            return "[ " + stringForCSV(interval.second.first) + " ; " + stringForCSV(interval.second.second) + " ] (" + stringForCSV(interval.first) + "%)";

    } else {
        if (formatFunc)
            return "[ " + formatFunc(interval.second.first, false) + " ; " + formatFunc(interval.second.second, false) + " ] (" + stringForLocal(interval.first) + "%)";

        else
            return "[ " + stringForLocal(interval.second.first) + " ; " + stringForLocal(interval.second.second) + " ] (" + stringForLocal(interval.first) + "%)";

    }*/

}

QString getHPDText(const QMap<double, double>& hpd, double thresh, const QString& unit,  DateConversion conversionFunc, const bool forCSV)
{
    if (hpd.isEmpty() )
        return "";

    QList<QPair<double, QPair<double, double> > > intervals = intervalsForHpd(hpd, thresh);
    QStringList results;

    for (auto&& interval : intervals)
        results << intervalText(interval, conversionFunc, forCSV);

    QString result = results.join(", ");
    if (!unit.isEmpty())
        result += " " + unit;

    return result;
}

/**
 * @brief Extract intervals (QPair of date) and calcul the area corresponding, from a HPD QMap maded before
 */
QList<QPair<double, QPair<double, double> > > intervalsForHpd(const QMap<double, double>& hpd, double thresh)
{
    QList<QPair<double, QPair<double, double> >> intervals;

    if (hpd.isEmpty())
        return intervals;

    QMapIterator<double, double> it(hpd);
    bool inInterval = false;
    double lastKeyInInter (0.);
    QPair<double, double> curInterval;

    double areaTot= map_area(hpd);
    double lastValueInInter (0.);

    double areaCur (0.);
    it.toFront();
    while (it.hasNext()) {
        it.next();

        if (it.value() != 0. && !inInterval) {
            inInterval = true;
            curInterval.first = it.key();
            lastKeyInInter = it.key();
            lastValueInInter = it.value();
            areaCur = 0.; // start, not inside

        } else if (inInterval) {
            if ((it.value() == 0.) ) {
                inInterval = false;
                curInterval.second = lastKeyInInter;
                 QPair<double, QPair<double, double> > inter;
                if (curInterval.second != curInterval.first) {

                    inter.first = thresh * areaCur / areaTot;

                 } else {
                    inter.first = thresh * lastValueInInter / areaTot;

                 }
                inter.second = curInterval;
                  intervals.append(inter);
                  areaCur = 0.;


            } else {
                 areaCur += (lastValueInInter+it.value())/2 * (it.key()-lastKeyInInter);

                lastKeyInInter = it.key();
                lastValueInInter = it.value();

            }
        }
    }

    if (inInterval) { // Correction to close unclosed interval

        curInterval.second = lastKeyInInter;
        //areaCur += (lastValueInInter+it.value())/2 * (it.key()-lastKeyInInter);
        QPair<double, QPair<double, double> > inter;
        inter.first = thresh * areaCur / areaTot;
        inter.second = curInterval;

        intervals.append(inter);
    }

    return intervals;
}
