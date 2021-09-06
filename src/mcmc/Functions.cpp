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
#include <QElapsedTimer>

#include <errno.h>      /* errno, EDOM */
#include <fenv.h>
#include <exception>

// -----------------------------------------------------------------
//  sumP = Sum (pi)
//  sum = Sum (pi * xi)
//  sum2 = Sum (pi * xi^2)
// -----------------------------------------------------------------

/**
 * @brief Product a FunctionStat from a QMap
 * @todo Handle empty function case and null density case (pi = 0)
 * @todo use Welford's online algorithm
 * https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
 *
 * // Calculate the variance based on equations (15) and (16) on
            // page 216 of "The Art of Computer Programming, Volume 2",
            // Second Edition. Donald E. Knuth.  Addison-Wesley
            // Publishing Company, 1973.
*/
FunctionStat analyseFunction(const QMap<type_data, type_data> &aFunction)
{
    FunctionStat result;
    if (aFunction.isEmpty()) {
        result.max = (type_data)0.;
        result.mode = (type_data)0.;
        result.mean = (type_data)0.;
        result.std = (type_data)(-1.);
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

    //FunctionStat result;
    result.max = max;
    result.mode = mode;
    result.mean = (type_data)0.;
    result.std = (type_data)0.;

    if (sumP != 0) {
        result.mean = sum / sumP;
        type_data variance = (sum2 / sumP) - pow(result.mean, 2);

        if (variance < 0) {
            qDebug() << "WARNING : in analyseFunction() negative variance found : " << variance<<" return 0";
            variance = -variance;
        }

        result.std = sqrt(variance);
    }

    const double step = (aFunction.lastKey() - aFunction.firstKey()) / (double)aFunction.size();
    QVector<double> subRepart = calculRepartition(aFunction);
    result.quartiles = quartilesForRepartition(subRepart, aFunction.firstKey(), step);

    return result;
}

/**
 * @brief std_Koening Algorithm using the Koenig-Huygens formula, can induce a negative variance
 * @param data
 * @return
 */
type_data std_Koening(const QVector<type_data> &data)
{
    // Work with double precision here because sum2 might be big !

    const type_data s = sum<type_data>(data);
    const type_data s2 = sum2<type_data>(data);
    const type_data mean = s / data.size();
    const type_data variance = s2 / data.size() - mean * mean;

  //  TraceStat result= traceStatistic(data);
//qDebug() << "std_Koening comparaison calcul Knuth" << sqrt(variance) << result.std;

    if (variance < 0) {
        qDebug() << "WARNING : in std_Koening() negative variance found : " << variance<<" force return 0";
        return (type_data)0.;
    }

    return sqrt(variance);
}


/**
 * @brief std_Knuth : Donald E. Knuth (TAOCP) (1998). The Art of Computer Programming, volume 2:
 * Seminumerical Algorithms, 3rd edn., p. 232. Boston: Addison-Wesley.
 * This algorithm was found by Welford:
 * Welford, B. P. (1962). "Note on a method for calculating corrected sums of squares and products".
 * Technometrics. 4 (3): 419–420. doi:10.2307/1266577. JSTOR 1266577.
 * @param data
 * @return
 */
type_data std_Knuth(const QVector<type_data> &data)
{
    int n = 0;
    type_data mean = 0.;
    type_data variance = 0.;
    type_data previousMean = 0.;
    type_data previousVariance = 0.;

    for (auto&& x : data) {
        n++;
        previousMean = std::move(mean);
        previousVariance = std::move(variance);
        mean = previousMean + (x - previousMean)/n;
        variance = previousVariance + (x - previousMean)*(x - mean);
    }

    return sqrt(variance/n);

}


double std_Knuth(const std::vector<int>& data)
{
    int n = 0;
    double mean = 0.;
    double variance = 0.;
    double previousMean = 0.;
    double previousVariance = 0.;

    for (auto&& x : data) {
        n++;
        previousMean = std::move(mean);
        previousVariance = std::move(variance);
        mean = previousMean + ((double)x - previousMean) / n;
        variance = previousVariance + ( (double)x - previousMean)*( (double)x - mean);
    }

    return sqrt(variance/n);

}


void mean_std_Knuth(const std::vector<int>& data, double& mean, double& std)
{
    int n = 0;
    double variance = 0.;
    double previousMean = 0.;
    double previousVariance = 0.;

    for (auto&& x : data) {
        n++;
        previousMean = std::move(mean);
        previousVariance = std::move(variance);
        mean = previousMean + ((double)x - previousMean) / n;
        variance = previousVariance + ( (double)x - previousMean)*( (double)x - mean);
    }

    std = std::move(sqrt(variance/n));

}

/**
 * @brief traceStatistic : This function uses the Knuth-Welford algorithm to calculate the standard deviation.
 * @param trace
 * @return
 */
TraceStat traceStatistic(const QVector<type_data>& trace)
{
    TraceStat result;
    int n = 0;
    type_data mean = 0.;
    type_data variance = 0.;
    type_data previousMean = 0.;
    type_data previousVariance = 0.;

    for (auto&& x : trace) {
        n++;
        previousMean = std::move(mean);
        previousVariance = std::move(variance);
        mean = previousMean + (x - previousMean)/n;
        variance = previousVariance + (x - previousMean)*(x - mean);
    }
    result.mean = std::move(mean);
    result.std = sqrt(variance/n);

    auto minMax = std::minmax_element(trace.begin(), trace.end());
    result.min = std::move(*minMax.first);
    result.max = std::move(*minMax.second);

    result.quartiles = quartilesForTrace(trace);
    return result;
}

double shrinkageUniform(const double so2)
{
    //double u = Generator::randomUniform();
    const double u = Generator::randomUniform(0, 1);
    return (so2 * (1. - u) / u);
}

/**
 * @brief Return a text from a FunctionStat
 * @see FunctionStat
 */
QString FunctionStatToString(const FunctionStat& analysis, const QString& nl, const bool forCSV)
{
    QString result;

    if (analysis.std<0.)
        result = QObject::tr("No data");

    else
        if (forCSV) {
            result += QObject::tr("MAP = %1  ;  Mean = %2  ;  Std = %3").arg( stringForCSV(analysis.mode),
                                                                              stringForCSV(analysis.mean),
                                                                              stringForCSV(analysis.std)) + nl;
            result += QObject::tr("Q1 = %1  ;  Q2 = %2  ;  Q3 = %3").arg( stringForCSV(analysis.quartiles.Q1),
                                                                          stringForCSV(analysis.quartiles.Q2),
                                                                          stringForCSV(analysis.quartiles.Q3));
        }  else {
            result += QObject::tr("MAP = %1  ;  Mean = %2  ;  Std = %3").arg( stringForLocal(analysis.mode),
                                                                              stringForLocal(analysis.mean),
                                                                              stringForLocal(analysis.std)) + nl;
            result += QObject::tr("Q1 = %1  ;  Q2 = %2  ;  Q3 = %3").arg( stringForLocal(analysis.quartiles.Q1),
                                                                          stringForLocal(analysis.quartiles.Q2),
                                                                          stringForLocal(analysis.quartiles.Q3));
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
    if (analysis.funcAnalysis.std>=0.) {


        if (forCSV) {
            result =  QObject::tr("Trace Stat.") + nl;
            result += QObject::tr("Mean = %1  ;  Std = %2").arg( stringForCSV(analysis.traceAnalysis.mean),
                                                                 stringForCSV(analysis.traceAnalysis.std)) + nl;
            result += QObject::tr("Q1 = %1  ;  Q2 (Median) = %2  ;  Q3 = %3 ").arg( stringForCSV(analysis.traceAnalysis.quartiles.Q1),
                                                                                    stringForCSV(analysis.traceAnalysis.quartiles.Q2),
                                                                                    stringForCSV(analysis.traceAnalysis.quartiles.Q3)) + nl;
            result +=  QObject::tr("min = %1  ;  max  = %2 ").arg( stringForCSV(analysis.traceAnalysis.min),
                                                                       stringForCSV(analysis.traceAnalysis.max)) + nl;
            result += nl + QObject::tr("Density Stat.") +  nl;


        } else {
            result = "<i>" + QObject::tr("Trace Stat.")  + "</i>"+ nl;
            result += QObject::tr("Mean = %1  ;  Std = %2").arg( stringForLocal(analysis.traceAnalysis.mean),
                                                                 stringForLocal(analysis.traceAnalysis.std)) + nl;
            result += QObject::tr("Q1 = %1  ;  Q2 (Median) = %2  ;  Q3 = %3 ").arg( stringForLocal(analysis.traceAnalysis.quartiles.Q1),
                                                                                    stringForLocal(analysis.traceAnalysis.quartiles.Q2),
                                                                                    stringForLocal(analysis.traceAnalysis.quartiles.Q3)) + nl;
            result += QObject::tr("min = %1  ;  max  = %2 ").arg( stringForLocal(analysis.traceAnalysis.min),
                                                                       stringForLocal(analysis.traceAnalysis.max)) + nl;
            result += nl + "<i>" + QObject::tr("Density Stat.") + "</i>"+ nl;
        }

        result += FunctionStatToString(analysis.funcAnalysis, nl, forCSV) + nl;

    }
    return result;
}


Quartiles quartilesForTrace(const QVector<type_data> &trace)
{
    //Q_ASSERT(&trace);
    Quartiles quartiles = quartilesType(trace, 8, 0.25);
    return quartiles;
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
    QElapsedTimer startTime;
    startTime.start();
#endif
    // limit of precision, to accelerate the calculus
    const double epsilonStep = 0.1/100.;

    // if thresh is equal 0 then return an QPair=(-INFINITY,+INFINITY)

    const int n = trace1.size();

    if ( (thresh > 0) && (n > 0) && ((int)trace2.size() == n) ) {

        const double gamma = 1. - thresh/100.;

        double dMin = INFINITY;

        std::vector<double> traceAlpha (std::vector<double>(trace1.cbegin(), trace1.cend()) );
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
    qDebug()<<"timeRangeFromTraces done in " + DHMS(startTime.elapsed());
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
    QElapsedTimer startTime ;
    startTime.start();
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
    //qDebug()<<description;
    QTime timeDiff(0, 0, 0, (int) startTime.elapsed());
    //timeDiff = timeDiff.addMSecs(startTime.elapsed()).addMSecs(-1);
    qDebug()<<"gapRangeFromTraces ->time elapsed = "<<timeDiff.hour()<<"h "<<QString::number(timeDiff.minute())<<"m "<<QString::number(timeDiff.second())<<"s "<<QString::number(timeDiff.msec())<<"ms" ;
#endif

    return range;
}

/**
 * @brief gapRangeFromTraces_old
 * @param traceBeta
 * @param traceAlpha
 * @param thresh
 * @param description : Obsolete function keep only for memory and test
 * @return
 */
QPair<float, float> gapRangeFromTraces_old(const QVector<float>& traceBeta, const QVector<float>& traceAlpha, const float thresh, const QString description)
{
    (void) description;
#ifdef DEBUG
    QElapsedTimer startTime ;
    startTime.start();
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
    //qDebug()<<description;
    QTime timeDiff(0, 0, 0, (int)startTime.elapsed() );
    //timeDiff = timeDiff.addMSecs(startTime.elapsed()).addMSecs(-1);
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

#pragma mark Calcul Matriciel

// useless function
std::vector<double> initVector(const unsigned n)
{
     return std::vector<double>(n, 0.);
}

std::vector<long double> initLongVector(const unsigned n)
{
     return std::vector<long double>(n, 0.);
}

std::vector<std::vector<double>> initMatrix(const unsigned rows, const unsigned cols)
{
    return std::vector<std::vector<double>> (rows, std::vector<double>(cols, 0.));
}

std::vector<std::vector<long double>> initLongMatrix(const unsigned rows, const unsigned cols)
{
   return std::vector<std::vector<long double>> (rows, std::vector<long double>(cols, 0.));
}

void resizeMatrix(std::vector<std::vector<double>>&matrix,  const unsigned rows, const unsigned cols)
{
    matrix.resize( rows );
    for ( std::vector<std::vector<double> >::iterator it = matrix.begin(); it != matrix.end(); ++it) {
        it->resize( cols );
    }
}

void resizeLongMatrix(std::vector<std::vector<long double>>&matrix,  const unsigned rows, const unsigned cols)
{
    matrix.resize( rows );
    for ( std::vector<std::vector<long double> >::iterator it = matrix.begin(); it != matrix.end(); ++it) {
        it->resize( cols );
    }
}

std::vector<std::vector<long double> > seedMatrix(const std::vector<std::vector<long double>>& matrix, const int shift)
{
    std::vector<std::vector<long double> > resMatrix ( matrix.at(0).size() - 2*shift );
    std::vector<std::vector<long double> >::iterator itRes = resMatrix.begin();
    for ( std::vector<std::vector<long double> >::const_iterator it = matrix.cbegin()+shift; it != matrix.cend()-shift; ++it) {
        *itRes = std::vector<long double>  ( it->cbegin() +shift, it->cend()-shift );
        ++itRes;
    }
    return resMatrix;
}

long double determinant(const std::vector<std::vector<long double>>& matrix, const int shift)
{
    int n = matrix.size();
    long double det;

    if (n - 2*shift == 1) {
          det = matrix.at(shift).at(shift);

    } else if (n - 2*shift == 2) {
        det = matrix.at(shift).at(shift) * matrix.at(1+shift).at(1+shift) - matrix.at(1+shift).at(shift) * matrix.at(shift).at(1+shift);

    } else {
        std::vector<std::vector<long double>> matrix2 = seedMatrix(matrix, shift);
        n = matrix2.size();

        std::vector<std::vector<long double>> matTmp (n-1, std::vector<long double>(n-1, 0.));

        det = 0.;
        int j2;
        for (int j1= 0; j1< n; j1++) {
            for (int i= 1; i< n; i++) {
                j2 = 0;
                for (int j = 0; j < n; j++) {
                    if (j == j1)
                        continue;
                    matTmp[i-1][j2] = matrix2.at(i).at(j);
                    j2++;
                }
            }
            det += pow(-1.0, j1+2.0) * matrix2.at(0).at(j1) * determinant(matTmp, 0);

        }

    }
    if (det == 0) {
           throw std::runtime_error("Function::determinant det ==0");
       }
    return(det);
}


// On suppose une matrice carrée N*N
std::vector<std::vector<long double>> transpose0(const std::vector<std::vector<long double> > &A)
{
   const unsigned n = A.size();
   std::vector<std::vector<long double>> TA  (n, std::vector<long double>(n));

   std::vector<std::vector<long double>>::const_iterator Ai;
   std::vector<long double>::const_iterator Aij;
   int i, j;
   for ( i = 0, Ai = A.begin(); Ai != A.end(); Ai++, i++)
       for (j= 0, Aij = Ai->begin() ; Aij != Ai->end(); Aij++, j++)
           TA[j][i] = *Aij ;

   return TA;

}

std::vector<std::vector<long double>> transpose(const std::vector<std::vector<long double>>& matrix, const int nbBandes)
{
    const int dim = matrix.size();
    std::vector<std::vector<long double>> result = initLongMatrix(dim, dim);

    // calcul de la demi-largeur de bande
    const int bande = floor((nbBandes-1)/2);
    int j1;
    int j2;
    for (int i = 0; i < dim; ++i) {
        j1 = std::max(0, i - bande);
        j2 = std::min(dim-1, i + bande);

        for (int j = j1; j <= j2; ++j) {
            result[j][i] = matrix.at(i).at(j);
        }
    }
    return result;
}


std::vector<std::vector<long double>> multiMatParDiag(const std::vector<std::vector<long double>>& matrix, const std::vector<long double>& diag, const int nbBandes)
{
    const int dim = matrix.size();
    std::vector<std::vector<long double>> result = initLongMatrix(dim, dim);
    const int bande = floor((nbBandes-1)/2); // calcul de la demi-largeur de bande

    auto matrix_i = matrix.at(0);
    for (int i = 0; i < dim; ++i) {
        int j1 = std::max(0, i - bande);
        int j2 = std::min(dim-1, i + bande);
       /* if (j1 < 0) {
            j1 = 0;
        }
        if (j2 >= dim) {
            j2 = dim-1;
        }*/

        matrix_i = matrix.at(i);
        for (int j = j1; j <= j2; ++j) {
            result[i][j] = matrix_i.at(j) * diag.at(j);
        }
    }
    return result;
}

std::vector<std::vector<long double>> multiDiagParMat(const std::vector<long double>& diag, const std::vector<std::vector<long double>>& matrix, const int nbBandes)
{
    const int dim = matrix.size();
    std::vector<std::vector<long double>> result = initLongMatrix(dim, dim);
    const int bande = floor((nbBandes-1)/2); // calcul de la demi-largeur de bande

    for (int i = 0; i < dim; ++i) {
        int j1 = std::max(0, i - bande);
        int j2 = std::min(dim-1, i + bande);
     /*   if (j1 < 0) {
            j1 = 0;
        }
        if (j2 >= dim) {
            j2 = dim-1;
        }*/
        for (int j = j1; j <= j2; ++j) {
            result[i][j] = diag.at(i) * matrix.at(i).at(j);
        }
    }
    return result;
}

/**
 * @brief multiMatParVec - calcul différent du produit matrice par Diagonal
 * @param matrix
 * @param vec
 * @param nbBandes
 * @return
 */
std::vector<long double> multiMatParVec(const std::vector<std::vector<long double>>& matrix, const std::vector<long double>& vec, const int nbBandes)
{
    const int dim = vec.size();
    std::vector<long double> result (dim);//= initVecteur(dim);
    const int bande = floor((nbBandes-1)/2); // calcul de la demi-largeur de bande

    for (int i = 0; i < dim; ++i) {
        long double sum = 0.;
        int j1 = std::max(0, i - bande);
        int j2 = std::min(dim-1, i + bande);
    /*    if (j1 < 0) {
            j1 = 0;
        }
        if (j2 >= dim) {
            j2 = dim-1;
        }*/
        for (int j = j1; j <= j2; ++j) {
            sum += matrix.at(i).at(j) * vec.at(j);
        }
        result[i] = sum;
    }
    return result;
}

/**
 * @brief MCMCLoopCurve::addMatEtMat
 * @param matrix1 Band matrix
 * @param matrix2 Band matrix
 * @param nbBandes2 The total Bandwidth = k1+k2 of Band matrix 2
 * @return
 */
std::vector<std::vector<long double>> addMatEtMat0(const std::vector<std::vector<long double>>& matrix1, const std::vector<std::vector<long double>>& matrix2)
{
    const int dim = matrix1.size();

    std::vector<std::vector<long double>> result (matrix1);
    for (int i = 0; i < dim; ++i) {
         for (int j = 0; j < dim; ++j) {
            result[i][j] +=  matrix2.at(i).at(j);
        }
    }
    return result;
}

std::vector<std::vector<long double>> addMatEtMat(const std::vector<std::vector<long double>>& matrix1, const std::vector<std::vector<long double>>& matrix2, const int nbBandes2)
{
    const int dim = matrix1.size();
    const int k = floor((nbBandes2-1)/2); // calcul de la demi-largeur de bande

    /*std::vector<std::vector<double>> result = initMatrix(dim, dim);
    for (int i = 0; i < dim; ++i) {
        int j1 = std::max(0, i - bande);
        int j2 = std::min(dim-1, i + bande);
        for (int j = j1; j <= j2; ++j) {
            result[i][j] = matrix1.at(i).at(j) + matrix2.at(i).at(j);
        }
    }*/
    std::vector<std::vector<long double>> result (matrix1);
    for (int i = 0; i < dim; ++i) {
        int j1 = std::max(0, i - k);
        int j2 = std::min(dim-1, i + k);
       /* if (j1 < 0) {
            j1 = 0;
        }
        if (j2 >= dim) {
            j2 = dim-1;
        }*/
        for (int j = j1; j <= j2; ++j) {
            result[i][j] +=  matrix2.at(i).at(j);
        }
    }
    return result;
}

std::vector<std::vector<long double>> addIdentityToMat(const std::vector<std::vector<long double>>& matrix)
{
    const int dim = matrix.size();
    std::vector<std::vector<long double>> result (matrix);

    for (int i = 0; i < dim; ++i)
        result[i][i] += 1.;

    return result;
}

std::vector<std::vector<long double>> multiConstParMat(const std::vector<std::vector<long double>>& matrix, const double c, const int nbBandes)
{
    const int dim = matrix.size();
    std::vector<std::vector<long double>> result (matrix);//= initMatrix(dim, dim);
    const int bande = floor((nbBandes-1)/2); // calcul de la demi-largeur de bande

    for (int i = 0; i < dim; ++i) {
        int j1 = std::max(0, i - bande);
        int j2 = std::min(dim-1, i + bande);
        /*if (j1 < 0) {
            j1 = 0;
        }
        if (j2 >= dim) {
            j2 = dim-1;
        }*/
        for (int j = j1; j <= j2; ++j) {
            result[i][j] *= c ;//* matrix.at(i).at(j);
        }
    }
    return result;
}
// without optimization full matrix
std::vector<std::vector<long double>> multiMatParMat0(const std::vector<std::vector<long double>>& matrix1, const std::vector<std::vector<long double>>& matrix2)
{
    const int n = matrix1.size();
    std::vector<std::vector<long double>> result = initLongMatrix(n, n);
    std::vector<long double>::const_iterator itMat1;
    double sum;

    for (int i = 0; i < n; ++i) {
        itMat1 = matrix1.at(i).begin();

        for (int j = 0; j < n; ++j) {
           sum = 0;
            for (int k = 0; k < n; ++k) {
                sum += (*(itMat1 + k)) * matrix2.at(k).at(j);

            }
            result[i][j] = sum;
        }
    }
    return result;
}


std::vector<std::vector<long double>> multiMatParMat(const std::vector<std::vector<long double>>& matrix1, const std::vector<std::vector<long double>>& matrix2, const int nbBandes1, const int nbBandes2)
{
    const int dim = matrix1.size();
    std::vector<std::vector<long double>> result = initLongMatrix(dim, dim);

    const int bande1 = floor((nbBandes1-1)/2);
    const int bande2 = floor((nbBandes2-1)/2);
    const int bandeRes = bande1 + bande2;

    for (int i = 0; i < dim; ++i) {
        int j1 = std::max(0, i - bandeRes);
        int j2 = std::min(dim-1, i + bandeRes);
     /*   if (j1 < 0) {
            j1 = 0;
        }
        if (j2 >= dim) {
            j2 = dim-1;
        }*/

       /* size_t k1 = i - bande1;
        size_t k2 = i + bande1;

        if (k1 < 0) {
            k1 = 0;
        }
        if (k2 >= dim) {
            k2 = dim-1;
        } */
        int k1 = std::max(0, i - bande1);
        int k2 = std::min(dim-1, i + bande1);
        std::vector<long double>::const_iterator itMat1 = matrix1.at(i).begin();
        double sum;
        for (int j = j1; j <= j2; ++j) {
          /*  int k1 = i - bande1;
            int k2 = i + bande1;

            if (k1 < 0) {
                k1 = 0;
            }
            if (k2 >= dim) {
                k2 = dim-1;
            }
*/
            sum = 0;
            for (int k = k1; k <= k2; ++k) {
                sum += (*(itMat1 + k)) * matrix2.at(k).at(j);
            }
            result[i][j] = sum;
        }
    }
    return result;
}

/**
 * @brief inverseMatSym0 Cette procedure execute l'inversion d'une matrice en utilisant la
 * formule de la comatrice
 * @param matrix
 * @param shift
 * @return
 */
std::vector<std::vector<long double>> inverseMatSym0(const std::vector<std::vector<long double>>& matrix, const int shift)
{
    if (matrix.size() != matrix[0].size()) {
           throw std::runtime_error("Matrix is not quadratic");
       }
    std::vector<std::vector<long double>> matInv = initLongMatrix(matrix.size(), matrix.at(0).size());

    std::vector<std::vector<long double>> matrix2 = seedMatrix(matrix, shift);

    int n = matrix.size();
    std::vector<std::vector<long double>> matInv2 = comatrice0(matrix2);

    const long double det = determinant(matrix2);
    if (det == 0) {
           throw std::runtime_error("inverseMatSym0 det ==0");
       }
    for (int i = shift; i < n-shift; i++)
        for (int j = shift; j< n-shift; j++)
             matInv[i][j] = matInv2[i-shift][j-shift] / det;


    return matInv;
}


/**
**** Cette procédure effectue l'inversion d'une matrice symétrique  ****
**** définie positive : toutes les valeurs propres doivent être     ****
**** positives (Cf. Angot, page 177)                                ****
**** à partir d'une décomposition de Cholesky en Mat_L et mat_D     ****
**** Cf algorithme Hutchison et De Hoog (1985)                      ****
**** donné par Green et Silverman, 1994, p.34                       ****
**** Attention : il y a une faute dans le bouquin de Green...!      ****
**/
// inverse_Mat_sym dans RenCurve
std::vector<std::vector<long double>> inverseMatSym_origin(const std::vector<std::vector<long double> > &matrixLE,  const std::vector<long double> &matrixDE, const int nbBandes, const int shift)
{
    int dim = matrixLE.size();
    std::vector<std::vector<long double>> matInv = initLongMatrix(dim, dim);//initMatrice(dim, dim);
    int bande = floor((nbBandes-1)/2);

    matInv[dim-1-shift][dim-1-shift] = 1. / matrixDE[dim-1-shift];
    matInv[dim-2-shift][dim-1-shift] = -matrixLE[dim-1-shift][dim-2-shift] * matInv[dim-1-shift][dim-1-shift];
    matInv[dim-2-shift][dim-2-shift] = (1. / matrixDE[dim-2-shift]) - matrixLE[dim-1-shift][dim-2-shift] * matInv[dim-2-shift][dim-1-shift];

    // shift : décalage qui permet d'éliminer les premières et dernières lignes et colonnes
    for (int i=dim-3-shift; i>=shift; --i) {
        matInv[i][i+2] = -matrixLE[i+1][i] * matInv[i+1][i+2] - matrixLE[i+2][i] * matInv[i+2][i+2];
        matInv[i][i+1] = -matrixLE[i+1][i] * matInv[i+1][i+1] - matrixLE[i+2][i] * matInv[i+1][i+2];
        matInv[i][i] = (1. / matrixDE[i]) - matrixLE[i+1][i] * matInv[i][i+1] - matrixLE[i+2][i] * matInv[i][i+2];

        if (bande >= 3)  {
            for (int k=3; k<=bande; ++k) {
                if (i+k < (dim - shift))  {
                    matInv[i][i+k] = -matrixLE[i+1][i] * matInv[i+1][i+k] - matrixLE[i+2][i] * matInv[i+2][i+k];
                }
            }
        }
    }

    for (int i=shift; i<dim-shift; ++i)  {
        for (int j=i+1; j<=i+bande; ++j)  {
            if (j < (dim-shift))   {
                matInv[j][i] = matInv[i][j];
            }
        }
    }

    return matInv;
}

/*
std::vector<std::vector<long double>> inverseMatSym_old(const std::vector<std::vector<long double>>& matrixLE, const std::vector<long double>& matrixDE, const int nbBandes, const int shift)
{
    const int dim = matrixLE.size();
    std::vector<std::vector<long double>> matInv = initLongMatrix(dim, dim);
    const int bande = floor((nbBandes-1)/2);

    matInv[dim-1-shift][dim-1-shift] = 1. / matrixDE.at(dim-1-shift);
    matInv[dim-2-shift][dim-1-shift] = -matrixLE[dim-1-shift][dim-2-shift] * matInv[dim-1-shift][dim-1-shift];
    matInv[dim-2-shift][dim-2-shift] = (1. / matrixDE.at(dim-2-shift)) - matrixLE[dim-1-shift][dim-2-shift] * matInv[dim-2-shift][dim-1-shift];

    // shift : décalage qui permet d'éliminer les premières et dernières lignes et colonnes

    if (bande >= 3) {
        for (int i = dim-3-shift; i >= shift; --i) {
                matInv[i][i+2] = -matrixLE[i+1][i] * matInv[i+1][i+2] - matrixLE[i+2][i] * matInv[i+2][i+2];
                matInv[i][i+1] = -matrixLE[i+1][i] * matInv[i+1][i+1] - matrixLE[i+2][i] * matInv[i+1][i+2];
                matInv[i][i] = (1. / matrixDE.at(i)) - matrixLE[i+1][i] * matInv[i][i+1] - matrixLE[i+2][i] * matInv[i][i+2];

                for (int k = 3; k <= bande; ++k) {
                    if (i+k < (dim - shift)) {
                        matInv[i][i+k] = -matrixLE[i+1][i] * matInv[i+1][i+k] - matrixLE[i+2][i] * matInv[i+2][i+k];
                    }//else What we do?
                }
        }

     } else {
        for (int i = dim-3-shift; i >= shift; --i) {
            matInv[i][i+2] = -matrixLE[i+1][i] * matInv[i+1][i+2] - matrixLE[i+2][i] * matInv[i+2][i+2];
            matInv[i][i+1] = -matrixLE[i+1][i] * matInv[i+1][i+1] - matrixLE[i+2][i] * matInv[i+1][i+2];
            matInv[i][i] = (1. / matrixDE.at(i)) - matrixLE[i+1][i] * matInv[i][i+1] - matrixLE[i+2][i] * matInv[i][i+2];


            for (int k = 3; k <= bande; ++k) {
                if (i+k < (dim - shift)) {
                    matInv[i][i+k] = -matrixLE[i+1][i] * matInv[i+1][i+k] - matrixLE[i+2][i] * matInv[i+2][i+k];
                }//else What we do?
            }
        }
     }



    for (int i = shift; i < dim-shift; ++i) {
        for (int j = i+1; j <= i+bande; ++j) {
            if (j < (dim-shift)) {
                matInv[j][i] = matInv[i][j];
            } //else What we do?
        }
    }

      //On symétrise la matrice Mat_1, même si cela n'est pas nécessaire lorsque bande=2

    return matInv;
}
*/

std::vector<std::vector<long double>> inverseMatSym(const std::vector<std::vector<long double>>& matrixLE, const std::vector<long double>& matrixDE, const int nbBandes, const int shift)
{
    const int dim = matrixLE.size();
    std::vector<std::vector<long double>> matInv = initLongMatrix(dim, dim);
    const int bande = floor((nbBandes-1)/2);

    matInv[dim-1-shift][dim-1-shift] = 1. / matrixDE.at(dim-1-shift);
    matInv[dim-2-shift][dim-1-shift] = -matrixLE[dim-1-shift][dim-2-shift] * matInv[dim-1-shift][dim-1-shift];
    matInv[dim-2-shift][dim-2-shift] = (1. / matrixDE.at(dim-2-shift)) - matrixLE[dim-1-shift][dim-2-shift] * matInv[dim-2-shift][dim-1-shift];

    // shift : décalage qui permet d'éliminer les premières et dernières lignes et colonnes
  /*  for (int i = dim-3-shift; i >= shift; --i) {
        matInv[i][i+2] = -matrixLE[i+1][i] * matInv[i+1][i+2] - matrixLE[i+2][i] * matInv[i+2][i+2];
        matInv[i][i+1] = -matrixLE[i+1][i] * matInv[i+1][i+1] - matrixLE[i+2][i] * matInv[i+1][i+2];
        matInv[i][i] = (1. / matrixDE[i][i]) - matrixLE[i+1][i] * matInv[i][i+1] - matrixLE[i+2][i] * matInv[i][i+2];

        if (bande >= 3) {
            for (int k = 3; k <= bande; ++k) {
                if (i+k < (dim - shift)) {
                    matInv[i][i+k] = -matrixLE[i+1][i] * matInv[i+1][i+k] - matrixLE[i+2][i] * matInv[i+2][i+k];
                }//else What we do?
            }
        }//else What we do?
    }
 */
    if (bande >= 3) {
        for (int i = dim-3-shift; i >= shift; --i) {
                matInv[i][i+2] = -matrixLE[i+1][i] * matInv[i+1][i+2] - matrixLE[i+2][i] * matInv[i+2][i+2];
                matInv[i][i+1] = -matrixLE[i+1][i] * matInv[i+1][i+1] - matrixLE[i+2][i] * matInv[i+1][i+2];
                matInv[i][i] = (1. / matrixDE.at(i)) - matrixLE[i+1][i] * matInv[i][i+1] - matrixLE[i+2][i] * matInv[i][i+1];

                for (int k = 3; k <= bande; ++k) {
                    if (i+k < (dim - shift)) {
                        matInv[i][i+k] = -matrixLE[i+1][i] * matInv[i+1][i+k] - matrixLE[i+2][i] * matInv[i+2][i+k];
                    }//else What we do?
                }
        }

     } else {
        for (int i = dim-3-shift; i >= shift; --i) {
            matInv[i][i+2] = -matrixLE[i+1][i] * matInv[i+1][i+2] - matrixLE[i+2][i] * matInv[i+2][i+2];
            matInv[i][i+1] = -matrixLE[i+1][i] * matInv[i+1][i+1] - matrixLE[i+2][i] * matInv[i+1][i+2];
            matInv[i][i] = (1. / matrixDE.at(i)) - matrixLE[i+1][i] * matInv[i][i+1] - matrixLE[i+2][i] * matInv[i][i+2];

          /*  const int kmin = std::min(bande, dim-shift-i);
            for (int k = 3; k <= kmin; ++k) {
                    matInv[i][i+k] = -matrixLE[i+1][i] * matInv[i+1][i+k] - matrixLE[i+2][i] * matInv[i+2][i+k];
            }
           */
            for (int k = 3; k <= bande; ++k) {
                if (i+k <= (dim-1 - shift)) {
                    matInv[i][i+k] = -matrixLE[i+1][i] * matInv[i+1][i+k] - matrixLE[i+2][i] * matInv[i+2][i+k];
                }//else What we do?
            }
        }
     }

    /* Code RenCurve
     * for i:=(dim-2-dc) downto 1+dc do begin
    Mat_1[i,i+2]:= -Mat_L_E[i+1,i]*Mat_1[i+1,i+2] - Mat_L_E[i+2,i]*Mat_1[i+2,i+2];
    Mat_1[i,i+1]:= -Mat_L_E[i+1,i]*Mat_1[i+1,i+1] - Mat_L_E[i+2,i]*Mat_1[i+1,i+2];
    Mat_1[i,i]  := (1/Mat_D_E[i,i]) - Mat_L_E[i+1,i]*Mat_1[i,i+1] - Mat_L_E[i+2,i]*Mat_1[i,i+2];
    if (bande>=3) then begin
      for k:=3 to bande do begin
        if (i+k<=(dim-dc)) then begin
          Mat_1[i,i+k]:= -Mat_L_E[i+1,i]*Mat_1[i+1,i+k] - Mat_L_E[i+2,i]*Mat_1[i+2,i+k];
        end;
      end;
    end;
  end;
     */

    for (int i = shift; i <= (dim - 1 -shift); ++i) {
        for (int j = i+1; j <= i+bande; ++j) {
            if (j <= (dim - 1 -shift)) {
                matInv[j][i] = matInv[i][j];
            } //else What we do?
        }
    }

      //On symétrise la matrice Mat_1, même si cela n'est pas nécessaire lorsque bande=2
  /*  for (int i = shift; i < dim-shift-1; ++i) {
        for (int j = i+1; j <= std::min(i+bande, dim-shift-1) ; ++j) {
                matInv[j][i] = matInv.at(i).at(j);
        }
    }
    */
    return matInv;
}

/* Never used !!!
 */
double sumAllMatrix(const std::vector<std::vector<double>>& matrix)
{
    double sum = 0.;
    for (unsigned long i = 0; i < matrix.size(); ++i) {
        sum += std::accumulate(matrix.at(i).begin(), matrix.at(i).end(), 0.);
    }
    return sum;
}

/* Never used !!!
 */
double sumAllVector(const std::vector<double>& vector)
{
    return std::accumulate(vector.begin(), vector.end(), 0.);
}

std::vector<std::vector<long double> > cofactor0(const std::vector<std::vector<long double>>& matrix)
{
    const int n = matrix.size();
    std::vector<std::vector<long double>> result = initLongMatrix(n, n);
    std::vector<std::vector<long double>> matMinorTmp = initLongMatrix(n-1, n-1);
    long double det;
    int i1, k1;
    for (int j=0; j<n; j++) {
        for (int i=0; i<n; i++) {
            i1 = 0;
            for (int k=0; k<n; k++) {
                if (k == i)
                    continue;
                k1 = 0;
                for (int l=0; l<n; l++) {
                    if (l == j)
                        continue;
                    matMinorTmp[i1][k1] = matrix.at(k).at(l);
                    k1++;
                }
                i1++;
            }

            det = determinant(matMinorTmp);

            result[i][j] = pow(-1., i+j+2.) * det;
        }
    }
    return result;
}

/**
 * @brief comatrice0 est la transposé de la matrice des cofactors
 * @param matrix
 * @return
 */
std::vector<std::vector<long double>> comatrice0(const std::vector<std::vector<long double>>& matrix)
{
    const int n = matrix.size();
    std::vector<std::vector<long double>> result = initLongMatrix(n, n);
    std::vector<std::vector<long double>> matMinorTmp = initLongMatrix(n-1, n-1);
    long double det;
    int i1, k1;
    for (int j=0; j<n; j++) {
        for (int i=0; i<n; i++) {
            i1 = 0;
            for (int k=0; k<n; k++) {
                if (k == i)
                    continue;
                k1 = 0;
                for (int l=0; l<n; l++) {
                    if (l == j)
                        continue;
                    matMinorTmp[i1][k1] = matrix.at(k).at(l);
                    k1++;
                }
                i1++;
            }

            det = determinant(matMinorTmp);

            result[j][i] = pow(-1., i+j+2.) * det;
        }
    }
    return result;
}

/**
 * @brief cholesky0
 * @param matrix
 * @return matrix triangulaire supérieur
 */

std::vector<std::vector<long double> > choleskyLL0(const std::vector<std::vector<long double> > &matrix)
{
    const int n = matrix.size();

    std::vector<std::vector<long double>> L = initLongMatrix(n, n);
    long double sum;

    for (int i=0; i<n; i++) {
            sum = matrix.at(i).at(i);
            for (int k=0; k<i; k++)
                sum -= pow(L.at(i).at(k), 2.);


            L[i][i] = sqrt(sum);

            for (int j=i+1; j<n; j++) {
                sum = matrix.at(i).at(j);

                for (int k=0; k<j-1; k++)
                    sum -=  L.at(j).at(k) * L.at(i).at(k);


                L[j][i] = sum / L.at(i).at(i);
            }
        }


    return L;
}

/**
 * @brief choleskyLDL
 * @param matrix
 * @return pair of 2 matrix
 */
std::pair<std::vector<std::vector<long double> >, std::vector<long double> > choleskyLDLT(const std::vector<std::vector<long double>>& matrix)
{
    // fonction à controler
    const int n = matrix.size();

    std::vector<std::vector<long double>> L = initLongMatrix(n, n);
    std::vector<long double> D = initLongVector(n);

    for (int i=0; i<n; i++) {
            L[i][i] = 1;
            for (int j=0; j<i; j++) {
                L[i][j] = matrix.at(i).at(j);
                for (int k=0; k<j; k++) {
                   L[i][j] -=  L.at(i).at(k) * L.at(j).at(k) *D.at(k);
                }
                L[i][j] /= D.at(j);
            }
            D[i] = matrix.at(i).at(i);
            for (int j=0; j<i; j++)
                D[i] -=  D.at(j) * pow(L.at(i).at(j), 2.);
      }





    return std::pair<std::vector<std::vector<long double>>, std::vector<long double>>(L, D);
}

/** ****************************************************************************
**** Cette procedure effectue la decomposition de Cholesky                 ****
**** sur la matrice passe en parametre (para)                              ****
**** Cf algorithme donné par S.M.Kay, "Modern Spectral Analysis"           ****
**** 1988, page 30.                                                        ****
**** Cette décomposition ne peut s'appliquer qu'à des matrices symétriques ****
*******************************************************************************/
std::pair<std::vector<std::vector<long double>>, std::vector<long double>> decompositionCholesky(const std::vector<std::vector<long double>>& matrix, const int nbBandes, const int shift)
{
    errno = 0;
      //if (math_errhandling & MATH_ERREXCEPT) feclearexcept(FE_ALL_EXCEPT);

    const int dim = matrix.size();
    std::vector<std::vector<long double>> matL = initLongMatrix(dim, dim);
    std::vector<long double> matD = initLongVector(dim);

   // const int bande = floor((nbBandes-1)/2);

    // shift : décalage qui permet d'éliminer les premières et dernières lignes et colonnes
    for (int i = shift; i < dim-shift; ++i) {
        matL[i][i] = 1.;
    }
    matD[shift] = matrix.at(shift).at(shift);

try {
    for (int i = shift+1; i < dim-shift; ++i) {
        matL[i][shift] = matrix.at(i).at(shift) / matD.at(shift);
     /*   avec bande */
        for (int j = shift+1; j < i; ++j) {
            if (abs(i - j) <= nbBandes) {
                long double sum = 0.;
                for (int k = shift; k < j; ++k) {
                    if (abs(i - k) <= nbBandes) {
                        sum += matL.at(i).at(k) * matD.at(k) * matL.at(j).at(k);
                    }
                }
                matL[i][j] = (matrix.at(i).at(j) - sum) / matD.at(j);
            }
        }

        long double sum = 0.;
        for (int k = shift; k < i; ++k) {
            if (abs(i - k) <= nbBandes) {
                sum += std::pow(matL.at(i).at(k), 2.) * matD.at(k);
            }
        }

        /* sans gestion de bande */
 /*       for (int j = shift+1; j < i; ++j) {

                long double sum = 0.;
                for (int k = shift; k < j; ++k) {
                        sum += matL.at(i).at(k) * matD.at(k) * matL.at(j).at(k);
                }
                matL[i][j] = (matrix.at(i).at(j) - sum) / matD.at(j);

        }

        double sum = 0.;
        for (int k = shift; k < i; ++k) {
                sum += std::pow(matL.at(i).at(k), 2.) * matD.at(k);

        }
*/
        matD[i] = matrix.at(i).at(i) - sum; // doit être positif
        if (matD.at(i) < 0) {
            qDebug() << "Function::decompositionCholesky : matD <0 change to 1E-200";
            matD[i] = 1e-200;
           // throw "Function::decompositionCholesky() matD <0";
        }
    }
    // matL : Par exemple pour n = 5 et shift =0:
    // 1 0 0 0 0
    // X 1 0 0 0
    // X X 1 0 0
    // X X X 1 0
    // X X X X 1

    // matL : Par exemple pour n = 5 et shift =1:
    // 0 0 0 0 0
    // 0 1 0 0 0
    // 0 X 1 0 0
    // 0 X X 1 0
    // 0 0 0 0 0
/*
    if (math_errhandling & MATH_ERRNO) {
        if (errno==EDOM)
            qDebug()<<"errno set to EDOM";
      }
      if (math_errhandling  &MATH_ERREXCEPT) {
        if (fetestexcept(FE_INVALID))
            qDebug()<<"decompositionCholesky -> FE_INVALID raised : Domain error: At least one of the arguments is a value for which the function is not defined.";
      }
*/
    } catch(...) {
                qDebug() << "Function::decompositionCholesky : Caught Exception!\n";
            }
    return std::pair<std::vector<std::vector<long double>>, std::vector<long double>>(matL, matD);
}

std::vector<long double> resolutionSystemeLineaireCholesky(const std::vector<std::vector<long double> > &matL, const std::vector<long double> &matD, const std::vector<long double> &vecQtY)
{
    const int n = matD.size(); //mModel->mEvents.size(); // pHd :: ?? est-ce que matD à la même dimension que mEvent??
    std::vector<long double> vecGamma (n);
    std::vector<long double> vecU (n);
    std::vector<long double> vecNu (n);


    vecU[1] = vecQtY.at(1);
    vecU[2] = vecQtY.at(2) - matL.at(2).at(1) * vecU.at(1);

    for (int i = 3; i < n-1; ++i) {
        vecU[i] = vecQtY.at(i) - matL.at(i).at(i-1) * vecU.at(i-1) - matL.at(i).at(i-2) * vecU.at(i-2); // pHd : Attention utilisation des variables déjà modiiées
    }

    for (int i = 1; i < n-1; ++i) {
        vecNu[i] = vecU.at(i) / matD.at(i);
    }

    vecGamma[n-2] = vecNu.at(n-2);
    vecGamma[n-3] = vecNu.at(n-3) - matL.at(n-2).at(n-3) * vecGamma.at(n-2);

    for (int i = n-4; i > 0; --i) {
        vecGamma[i] = vecNu.at(i) - matL.at(i+1).at(i) * vecGamma.at(i+1) - matL.at(i+2).at(i) * vecGamma.at(i+2); // pHd : Attention utilisation des variables déjà modiiées
    }
    return vecGamma;
}


/**
 * @brief Strassen::sub substraction of 2 square matrix NxN
 * @param A square matrix NxN
 * @param B square matrix NxN
 * @return one square matrix NxN
 */
std::vector<std::vector<long double>> Strassen::sub(const std::vector<std::vector<long double>>&  A, const std::vector<std::vector<long double>>& B)
{
    const int n = A.size();

    std::vector<std::vector<long double>> C (n, std::vector<long double>(n, 0.));

  /*  for (int i = 0; i < n; i++)

        for (int j = 0; j < n; j++)

            C[i][j] = A[i][j] - B[i][j];
*/
    std::vector<std::vector<long double>>::iterator ci = C.begin();
    std::vector<long double>::iterator cij;

    std::vector<std::vector<long double>>::const_iterator ai, bi;
    std::vector<long double>::const_iterator aij, bij;

    for (ai = A.begin(), bi =B.begin() ; ai != A.end(); ++ai, ++bi) {
        cij = ci->begin();
        for (aij = ai->begin(), bij = bi->begin() ; aij != ai->end(); ++aij, ++bij) {
            *cij = *aij - *bij;
            ++cij;
        }
        ++ci;
    }

    return C;
}

/**
 * @brief Strassen::add addition of 2 square matrix NxN
 * @param A square matrix NxN
 * @param B square matrix NxN
 * @return one square matrix NxN
 */
std::vector<std::vector<long double>> Strassen::add(const std::vector<std::vector<long double>>& A, const std::vector<std::vector<long double>>& B)
{
    const int n = A.size();

   std::vector<std::vector<long double>> C (n, std::vector<long double>(n, 0.));

   /* for (int i = 0; i < n; i++)
   for (int j = 0; j < n; j++)
       C[i][j] = A[i][j] + B[i][j];
   */
   std::vector<std::vector<long double>>::iterator ci = C.begin();
   std::vector<long double>::iterator cij;

   std::vector<std::vector<long double>>::const_iterator ai, bi;
   std::vector<long double>::const_iterator aij, bij;

   for (ai = A.begin(), bi =B.begin() ; ai != A.end(); ++ai, ++bi) {
       cij = ci->begin();
       for (aij = ai->begin(), bij = bi->begin() ; aij != ai->end(); ++aij, ++bij) {
           *cij = *aij + *bij;
           ++cij;
       }
       ++ci;
   }
   return C;
}
/** Funtion to split parent matrix into child matrices **/

void Strassen::split(const std::vector<std::vector<long double>>& P,  std::vector<std::vector<long double>>& C, int iB, int jB)
{
    const int lig = C.size();
    const int col = C[0].size();

    for (int i1 = 0, i2 = iB; i1 < lig; ++i1, ++i2)
        for (int j1 = 0, j2 = jB; j1 < col; ++j1, ++j2)
            C[i1][j1] = P.at(i2).at(j2);

}
void Strassen::join(const std::vector<std::vector<long double>>& C,  std::vector<std::vector<long double>>& P, int iB, int jB)
{
    const int lig = C.size();
    const int col = C[0].size();
    for (int i1 = 0, i2 = iB; i1 < lig; ++i1, ++i2)

        for (int j1 = 0, j2 = jB; j1 < col; ++j1, ++j2)
            P[i2][j2] = C[i1][j1];
}

/**
 * @brief Strassen::multiply  2 square matrix of same size NxN with 17 additions and 7 multiplications
 * @ref Strassen, Volker, Gaussian Elimination is not Optimal, Numer. Math. 13, p. 354-356, 1969
 * original code : https://www.sanfoundry.com/java-program-strassen-algorithm/
 * @param A square matrix size NxN
 * @param B square matrix size NxN
 * @return square matrix size NxN
 */
std::vector<std::vector<long double>> Strassen::multiply (const std::vector<std::vector<long double>>& A, const std::vector<std::vector<long double>>& B)
{
    const int n = A.size();
    std::vector<std::vector<long double>> R;

    /** base case **/

    if (n == 1) {
        R = initLongMatrix(1, 1);
        R[0][0] = A.at(0).at(0) * B.at(0).at(0);

     } else {
        // controle de la puissance de n, n doit être une puissance de 2
        int nP2;

        std::vector<std::vector<long double>> A2;
        std::vector<std::vector<long double>> B2;

        if ( n != pow( 2., floor(log(n)/log(2))) ) { //null row and column filling
            nP2 = pow( 2., floor(log(n)/log(2)) + 1);
            A2 = initLongMatrix(nP2, nP2);
            B2 = initLongMatrix(nP2, nP2);

            join (A, A2, 0, 0);
            join (B, B2, 0, 0);

        } else {
            A2 = A;
            B2 = B;
            nP2 = n;
        }

        const int n1 = floor(nP2/2);

        std::vector<std::vector<long double>> A11 (n1, std::vector<long double>(n1, 0.));
        std::vector<std::vector<long double>> A12 (n1, std::vector<long double>(n1, 0.));
        std::vector<std::vector<long double>> A21 (n1, std::vector<long double>(n1, 0.));
        std::vector<std::vector<long double>> A22 (n1, std::vector<long double>(n1, 0.));

        std::vector<std::vector<long double>> B11 (n1, std::vector<long double>(n1, 0.));
        std::vector<std::vector<long double>> B12 (n1, std::vector<long double>(n1, 0.));
        std::vector<std::vector<long double>> B21 (n1, std::vector<long double>(n1, 0.));
        std::vector<std::vector<long double>> B22 (n1, std::vector<long double>(n1, 0.));



        /** Dividing matrix A into 4 halves **/

        split(A2, A11, 0 , 0);
        split(A2, A12, 0 , n1);
        split(A2, A21, n1, 0);
        split(A2, A22, n1, n1);

        /** Dividing matrix B into 4 halves **/

        split(B2, B11, 0 , 0);
        split(B2, B12, 0 , n1);
        split(B2, B21, n1, 0);
        split(B2, B22, n1, n1);



        /**

          M1 = (A11 + A22)(B11 + B22)

          M2 = (A21 + A22) B11

          M3 = A11 (B12 - B22)

          M4 = A22 (B21 - B11)

          M5 = (A11 + A12) B22

          M6 = (A21 - A11) (B11 + B12)

          M7 = (A12 - A22) (B21 + B22)

        **/



        std::vector<std::vector<long double>> M1 = multiply(add(A11, A22), add(B11, B22));

        std::vector<std::vector<long double>> M2 = multiply(add(A21, A22), B11);

        std::vector<std::vector<long double>> M3 = multiply(A11, sub(B12, B22));

        std::vector<std::vector<long double>> M4 = multiply(A22, sub(B21, B11));

        std::vector<std::vector<long double>> M5 = multiply(add(A11, A12), B22);

        std::vector<std::vector<long double>> M6 = multiply(sub(A21, A11), add(B11, B12));

        std::vector<std::vector<long double>> M7 = multiply(sub(A12, A22), add(B21, B22));



        /**

          C11 = M1 + M4 - M5 + M7

          C12 = M3 + M5

          C21 = M2 + M4

          C22 = M1 - M2 + M3 + M6

        **/

        std::vector<std::vector<long double>> C11 = add(sub(add(M1, M4), M5), M7);
        std::vector<std::vector<long double>> C12 = add(M3, M5);
        std::vector<std::vector<long double>> C21 = add(M2, M4);
        std::vector<std::vector<long double>> C22 = add(sub(add(M1, M3), M2), M6);

        R = initLongMatrix(nP2, nP2);

        join(C11, R, 0 , 0);
        join(C12, R, 0 , n1);
        join(C21, R, n1, 0);
        join(C22, R, n1, n1);

        if (n != nP2) {
            resizeLongMatrix(R, n, n);
        }

    }

    /** return result **/

    return R;
}

/**
 * @brief decompositionLU0
 * example of good code https://livedu.in/lu-decomposition-method-algorithm-implementation-in-c-with-solved-examples/
 * @param A
 * @return
 */
std::pair<std::vector<std::vector<long double> >, std::vector<std::vector<long double>> > decompositionLU0(const std::vector<std::vector<long double>>& A)
{
   const unsigned n = A.size();

    std::vector<std::vector<long double>> L (n, std::vector<long double>(n, 0.));
    std::vector<std::vector<long double>> U (n, std::vector<long double>(n, 0.));

    unsigned i, j , k;

    for (i = 0; i < n; i++) {
        // Upper triangle
        for (k  = i; k < n; k++) {
            U[i][k] = A.at(i).at(k);
            for (j = 0; j < i; j++)
                U[i][k] -=  U.at(j).at(k) * L.at(i).at(j);
        }

        // diagonal creation
        L[i][i] = 1.;

        // Lower triangle
        for (k = i+1; k < n; k++) {
            L[k][i] = A.at(k).at(i);
            for (j = 0; j < i; j++)
                L[k][i] -=  U.at(j).at(i) * L.at(k).at(j);

            L[k][i] /= U.at(i).at(i);
        }


    }

    return std::pair<std::vector<std::vector<long double>>, std::vector<std::vector<long double>>>(L, U);
}
