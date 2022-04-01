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
#include <iostream>
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
 *  Calculate the variance based on equations (15) and (16) on
 *  page 216 of "The Art of Computer Programming, Volume 2",
 *  Second Edition. Donald E. Knuth.  Addison-Wesley
 *  Publishing Company, 1973.
*/

//https://fanf2.user.srcf.net/hermes/doc/antiforgery/stats.pdf
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

    type_data prev_y (0.);
    QList<type_data> uniformXValues;

    QMap<type_data,type_data>::const_iterator citer = aFunction.cbegin();

    /*
    type_data sum2 (0.);
    type_data sumP (0.);
    type_data sum (0.);
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

    result.mean = (type_data)0.;
    result.std = (type_data)0.;

    if (sumP != 0) {
        result.mean = sum / sumP;
        type_data variance = (sum2 / sumP) - pow(result.mean, 2);

        if (variance < 0) {
            qDebug() << "WARNING : in analyseFunction() negative variance found : " << variance<<" return 0";
            variance = -variance;
        }
*/

    type_data y_sum = 0;
    type_data mean = 0;
    type_data mean_prev;
    type_data  x , y;
    type_data variance = 0;
    for (;citer != aFunction.cend(); ++citer) {
        x = citer.key();
        y = citer.value();
        y_sum +=  y;

        mean_prev = mean;
        mean +=  (y / y_sum) * (x - mean_prev);
        variance += y * (x - mean_prev) * (x - mean);

        // find max
        if (max <= y) {
            max = y;
            if (prev_y == y) {
                uniformXValues.append(x);
                int middleIndex = floor(uniformXValues.size()/2);
                mode = uniformXValues.at(middleIndex);
            } else {
                uniformXValues.clear();
                mode = x;
            }
        }
        prev_y = y;
    }
    variance /=  y_sum;

    result.max = std::move(max);
    result.mode = std::move(mode);
    result.std = sqrt(variance);
    result.mean = std::move(mean);

    const double step = (aFunction.lastKey() - aFunction.firstKey()) / (double)(aFunction.size() - 1);
    QVector<double> subRepart = calculRepartition(aFunction);
    result.quartiles = quartilesForRepartition(subRepart, aFunction.firstKey(), step);

    return result;
}

/**
 * @brief std_Koening Algorithm using the Koenig-Huygens formula, can induce a negative variance
 * return std biaised, must be corrected if unbiais needed
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
double std_Knuth(const QVector<double> &data)
{
    unsigned n = 0;
    long double mean = 0.;
    long double variance = 0.;
    long double previousMean = 0.;
    long double previousVariance = 0.;

    for (auto&& x : data) {
        n++;
        previousMean = std::move(mean);
        previousVariance = std::move(variance);
        mean = previousMean + (x - previousMean)/n;
        variance = previousVariance + (x - previousMean)*(x - mean);
    }

    return sqrt(variance/(long double)n);

}

double std_Knuth(const std::vector<double> &data)
{
    unsigned n = 0;
    long double mean = 0.;
    long double variance = 0.;
    long double previousMean;
    long double previousVariance;

    for (auto&& x : data) {
        n++;
        previousMean = std::move(mean);
        previousVariance = std::move(variance);
        mean = previousMean + (x - previousMean)/n;
        variance = previousVariance + (x - previousMean)*(x - mean);
    }

    return sqrt(variance/(long double)n);

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

void mean_variance_Knuth(const std::vector<double>& data, double& mean, double& variance)
{
    int n = 0;
    variance = 0.;
    mean = 0.;
    double previousMean = 0.;
    double previousVariance = 0.;

    for (auto&& x : data) {
        n++;
        previousMean = std::move(mean);
        previousVariance = std::move(variance);
        mean = previousMean + (x - previousMean) / n;
        variance = previousVariance + ( x - previousMean)*( x - mean);
    }

}
void mean_variance_Knuth(const QVector<double>& data, double& mean, double& variance)
{
    int n = 0;
    variance = 0.;
    mean = 0.;
    double previousMean = 0.;
    double previousVariance = 0.;

    for (auto&& x : data) {
        n++;
        previousMean = std::move(mean);
        previousVariance = std::move(variance);
        mean = previousMean + (x - previousMean) / n;
        variance = previousVariance + ( x - previousMean)*( x - mean);
    }

}

double std_unbiais_Knuth (const QVector<double> &data)
{
    unsigned n = 0;
    double mean = 0.;
    double variance = 0.;
    double previousMean = 0.;
    double previousVariance = 0.;

    for (auto&& x : data) {
        n++;
        previousMean = std::move(mean);
        previousVariance = std::move(variance);
        mean = previousMean + (x - previousMean)/n;
        variance = previousVariance + (x - previousMean)*(x - mean);
    }

    return sqrt(variance/(double)(n-1)); // unbiais

}

double std_unbiais_Knuth(const std::vector<double> &data)
{
    unsigned n = 0;
    long double mean = 0.;
    long double variance = 0.;
    long double previousMean = 0.;
    long double previousVariance = 0.;

    for (auto&& x : data) {
        n++;
        previousMean = std::move(mean);
        previousVariance = std::move(variance);
        mean = previousMean + (x - previousMean)/n;
        variance = previousVariance + (x - previousMean)*(x - mean);
    }

    return sqrt(variance/(long double)(n-1));

}

double std_unbiais_Knuth(const std::vector<int>& data)
{
    int n = 0;
    long double mean = 0.;
    long double variance = 0.;
    long double previousMean = 0.;
    long double previousVariance = 0.;

    for (auto&& x : data) {
        n++;
        previousMean = std::move(mean);
        previousVariance = std::move(variance);
        mean = previousMean + ((double)x - previousMean) / n;
        variance = previousVariance + ( (double)x - previousMean)*( (double)x - mean);
    }

    return sqrt(variance/(n-1)); // unbiais

}

void mean_std_unbiais_Knuth(const std::vector<int>& data, double& mean, double& std)
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

    std = std::move(sqrt(variance/(n-1)));// unbiais

}

// dataX and dataY must have the same length
double covariance(const std::vector<double>& dataX, const std::vector<double>& dataY)
{
    double meanx = 0.;
    double meany = 0.;
    double covar = 0.;
    double n = 0;
    double dx;
    auto ptr_dX = dataX.begin();
    auto ptr_dY = dataY.begin();

    for (;ptr_dX != dataX.end(); ptr_dX++, ptr_dY++ ) {
        n++;
        dx = *ptr_dX - meanx;
        meanx += dx / n;
        meany += (*ptr_dY - meany) / n;
        covar += dx * (*ptr_dY - meany);
    }


    return covar / n;
    // Bessel's correction for sample variance
   // sample_covar = C / (n - 1)
}

// dataX and dataY must have the same length
/**
 * @brief linear_regression using knuth algorithm
 * @param dataX
 * @param dataY
 * @return a, b coef and constant $$Y = a*t + b$$
 */
const std::pair<double, double> linear_regression(const std::vector<double>& dataX, const std::vector<double>& dataY)
{
    double mean_x = 0.;
    double mean_y = 0.;
    double covar = 0.;
    double var_x = 0;
    double previousMean_x;
    double n = 0;
    double dx;
    auto ptr_x = dataX.begin();
    auto ptr_y = dataY.begin();

    for (;ptr_x != dataX.end(); ptr_x++, ptr_y++ ) {
        n++;
        previousMean_x = mean_x;

        dx = *ptr_x - mean_x;
        mean_x += dx / n;
        var_x += ( *ptr_x - previousMean_x)*( *ptr_x - mean_x);

        mean_y += (*ptr_y - mean_y) / n;
        covar += dx * (*ptr_y - mean_y);

    }

    //var_x /= n; // useless
    //covar /= n;

    double a = (covar / var_x);
    double b = mean_y - a*mean_x;

    return std::pair<double, double>(std::move(a), std::move(b));
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
    result.std = sqrt(variance/(n-1)); // unbiais

    auto minMax = std::minmax_element(trace.begin(), trace.end());
    result.min = std::move(*minMax.first);
    result.max = std::move(*minMax.second);

    result.quartiles = quartilesForTrace(trace);
    return result;
}

double shrinkageUniform(const double s02)
{
    //double u = Generator::randomUniform();
    const double u = Generator::randomUniform(0, 1);
    return (s02 * (1. - u) / u);
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
    for (auto&& v : calib) {
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
    for (auto&& v : repartitionTemp)
        v = v/lastRepVal;

    return repartitionTemp;
}
Quartiles quartilesForRepartition(const QVector<double>& repartition, const double tmin, const double step)
{
    Quartiles quartiles;
    if (repartition.size()<5) {
        quartiles.Q1 = 0.;
        quartiles.Q2 = tmin;
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
    size_t n = trace.size();
    if (thresh > 0 && n > 0) {
        double threshold = inRange(0.0, thresh, 100.0);
        QVector<double> sorted (trace);
        std::sort(sorted.begin(),sorted.end());

        size_t numToRemove = (size_t)floor(n * (1. - threshold / 100.));
        exactThresholdResult = ((double)n - (double)numToRemove) / (double)n;

        double lmin = 0.;
        int foundJ = 0;

        for (size_t j=0; j<=numToRemove; ++j) {
            const double l = sorted.at((n - 1) - numToRemove + j) - sorted.at(j);
            if ((lmin == 0.) || (l < lmin)) {
                foundJ = j;
                lmin = l;
            }
        }
        credibility.first = sorted.at(foundJ);
        credibility.second = sorted.at((n - 1) - numToRemove + foundJ);
    }

    if (credibility.first == credibility.second) {
        //It means : there is only one value
        exactThresholdResult = 1;
        //return QPair<double, double>();
    }

        return credibility;
}

// Used in generateTempo for credibility
QPair<double, double> credibilityForTrace(const QVector<int>& trace, double thresh, double& exactThresholdResult, const QString description)
{
    (void) description;
    QPair<double, double> credibility(0.,0.);
    exactThresholdResult = 0.;
    const int n = trace.size();
    if (thresh > 0 && n > 0) {
        double threshold = inRange(0.0, thresh, 100.0);
        QVector<int> sorted (trace);
        std::sort(sorted.begin(),sorted.end());

        unsigned numToRemove = (unsigned)floor(n * (1. - threshold / 100.));
        exactThresholdResult = ((double)n - (double)numToRemove) / (double)n;

        double lmin (0.);
        int foundJ (0);

        for (unsigned j=0; j<=numToRemove; ++j) {
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
        exactThresholdResult = 1;
        //return QPair<double, double>();
    }
    //else
        return credibility;
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

    unsigned n = trace1.size();

    if ( (thresh > 0) && (n > 0) && ((unsigned)trace2.size() == n) ) {

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

            const int betaUpperSize = (int) std::distance(betaUpper.begin(), it);

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

    const int  n = traceBegin.size();

    if ( (thresh > 0.f) && (n > 0) && (traceEnd.size() == n) ) {

        const double gamma = 1. - thresh/100.;

        double dMax(0.);

        // We must change the type (float to double) to increase the precision
        std::vector<double> traceBeta (traceEnd.size());
        std::copy(traceEnd.begin(), traceEnd.end(), traceBeta.begin());

        std::vector<double> traceAlpha (traceBegin.size());
        std::copy(traceBegin.begin(),traceBegin.end(),traceAlpha.begin());

        // 1 - map with relation Alpha to Beta
        std::multimap<double, double> alphaBeta;
        for (int i=0; i<traceBegin.size(); ++i)
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

            // 3 - Copy only value of beta with alpha smaller than a(epsilon)!
            const int alphaIdx(haSup < n ? haSup : n-1 );//( ha == haInf ? haInf : haSup );//( ha == haSup ? haSup : haInf );// //

            const int remainingElemt ( alphaIdx );
            alphaUnder.resize(remainingElemt);   // allocate space

            // traceAlpha is sorted with the value alpha join
            auto it = std::copy( traceAlpha.begin(), traceAlpha.begin() + alphaIdx, alphaUnder.begin() );

            const int alphaUnderSize = (int) std::distance(alphaUnder.begin(),it);

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

        const int nTarget = (int) ceil( (float)n * thresh/100.);
        const int nGamma = n - nTarget;

        float dMax = 0.0;

        // make couple beta vs alpha in a std::map, it's a sorted container with ">"
        std::multimap<float, float> mapPair;

        QVector<float>::const_iterator ctB = traceBeta.cbegin();
        QVector<float>::const_iterator ctA = traceAlpha.cbegin();

        while (ctB != traceBeta.cend() ) {
            mapPair.insert(std::pair<float, float>(*ctB,*ctA));
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
         return stringForCSV(inter1) + AppSettings::mCSVCellSeparator + stringForCSV(inter2) + AppSettings::mCSVCellSeparator + stringForCSV(perCent);
    else
         return "[ " + stringForLocal(interval.second.first) + " ; " + stringForLocal(interval.second.second) + " ] (" + stringForLocal(interval.first) + "%)";

}

QString getHPDText(const QMap<double, double>& hpd, double thresh, const QString& unit,  DateConversion conversionFunc, const bool forCSV)
{
    if (hpd.isEmpty() )
        return "";

    QList<QPair<double, QPair<double, double> > > intervals = intervalsForHpd(hpd, thresh);
    QStringList results;

    for (auto&& interval : intervals)
        results << intervalText(interval, conversionFunc, forCSV);

    QString result;
    if (forCSV) {
        result = results.join(AppSettings::mCSVCellSeparator);

    } else {
        result = results.join(", ");
        if (!unit.isEmpty())
                result += " " + unit;
    }

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
//std::cout<<"Function::intervalsForHpd"<<inter.first<< inter.second.first<<inter.second.second;

            } else {
                 areaCur += (lastValueInInter + it.value())/2 * (it.key() - lastKeyInInter);

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


/**
 * @brief intervalMonomodalHpd
 * Détermine le mode et un seul intervalle HPD pour une densité mono-modale
 * @param density
 * @param thresh
 * @return
 */
std::pair<int, std::pair<int, int> > intervalMonomodalHpd(const std::map<int, double>& density, double thresh)
{
    if (density.size() < 2)  // in case of only one value (e.g. a bound fixed) or no value
        return qMakePair(0, qMakePair(0, 0));
    // On considère ici une fonction histogramme, la surface est considèrée constante et chaque fréquence a une la même surface
    double areaTot = std::accumulate(density.cbegin(), density.cend(), 0., [](double sum, const std::pair<int, double> m){return sum + m.second;  });

    // Inversion de la map

    std::multimap<double, int> inverted;

    for (const auto& d : density) {
       inverted.insert(std::pair<double, int>(d.second, d.first));
    }

    // Par définition les maps sont triées, donc la dernière valeur est le mode, rbegin() pointe sur la derniere valeur

    int mode = inverted.rbegin()->second;


    // On fait la somme en remontant la map inversée, jusqu'à avoir la somme seuil

    typedef std::multimap<double, int>::iterator iter_type;
    std::reverse_iterator<iter_type> iterInverted (inverted.rbegin());

    double area = 0.;
    double areaSearched = areaTot * thresh / 100.;
    int imin = mode;
    int imax = mode;

    int lastIMin, lastIMax;
    if (areaTot == areaSearched) {
        return qMakePair(mode, qMakePair(density.cbegin()->first, density.rbegin()->first));;

    } else {

        try {

            while (iterInverted != inverted.rend() && area<=areaSearched) {

                int t = iterInverted->second;
                const double v = iterInverted->first;

                area += v;

                imin = std::min(imin, t);
                imax = std::max(imax, t);
                iterInverted++;
                lastIMax = imax;
                lastIMin = imin;
            }
            // on prend les indices avant que areaSearched soit dépassé
            return qMakePair(mode, qMakePair(lastIMin, lastIMax));
       }
       catch (std::exception const & e) {
            qDebug()<< "in Function::intervalMonomodalHpd int Error"<<e.what();
            return qMakePair(0, qMakePair(0, 0));;
       }
    }

}



#pragma mark Calcul Matriciel

// useless function
std::vector<double> initVector(size_t n)
{
     return std::vector<double>(n, 0.);
}

std::vector<long double> initLongVector(size_t n)
{
     return std::vector<long double>(n, 0.);
}


std::vector<std::vector<int>> initIntMatrix(size_t rows, size_t cols)
{
    return std::vector<std::vector<int>> (rows, std::vector<int>(cols, 0.));
}

std::vector<std::vector<double>> initMatrix(size_t rows, size_t cols)
{
    return std::vector<std::vector<double>> (rows, std::vector<double>(cols, 0.));
}


void resizeMatrix(std::vector<std::vector<double>> &matrix,  size_t rows, size_t cols)
{
    matrix.resize( rows );
    for ( auto&& it = matrix.begin(); it != matrix.end(); ++it) {
        it->resize( cols );
    }
}

void resizeLongMatrix(Matrix2D & matrix,  size_t rows, size_t cols)
{
    Matrix2D resMatrix ( rows );
    std::valarray<double>* itRes = begin(resMatrix);
    std::valarray<double>* it = begin(matrix);
    for ( ; it != end(matrix) && itRes != end(resMatrix) ; ++it, ++itRes) {
        *itRes = std::valarray<double>  ( (*it)[std::slice(0, cols, 1)] );
    }
    matrix = resMatrix;
 /*
    matrix.resize( rows );
    for ( auto it = begin(matrix); it != end(matrix); ++it) {
        it->resize( cols );
    }
    */
}

Matrix2D seedMatrix(const Matrix2D &matrix, size_t shift)
{
    if (shift == 0)
        return matrix;
    size_t n = matrix[0].size() - 2*shift;
    Matrix2D resMatrix ( n );
    auto itRes = begin(resMatrix);
    for ( auto it = begin(matrix)+shift; it != end(matrix)-shift; ++it) {
        //*itRes = std::valarray<long double>  ( begin(*it) +shift, end(*it)-shift );
        *itRes = std::valarray<double>  ( (*it)[std::slice(shift, n, 1)] );
        ++itRes;
    }
    return resMatrix;
}

double determinant(const Matrix2D &matrix, size_t shift)
{
 //https://askcodez.com/matrice-de-determinant-de-lalgorithme-de-c.html
    size_t n = matrix.size();
    double det;

    if (n - 2*shift == 1) {
          det = matrix[shift][shift];

    } else if (n - 2*shift == 2) {
        det = matrix[shift][shift] * matrix[1+shift][1+shift] - matrix[1+shift][shift] * matrix[shift][1+shift];

    } else {
        Matrix2D matrix2 = seedMatrix(matrix, shift);
        n = matrix2.size();

        Matrix2D matTmp = initMatrix2D(n-1, n-1);

        det = 0.;
        int j2;
        for (size_t j1= 0; j1< n; j1++) {
            for (size_t i= 1; i< n; i++) {
                j2 = 0;
                for (size_t j = 0; j < n; j++) {
                    if (j == j1)
                        continue;
                    matTmp[i-1][j2] = matrix2[i][j];
                    j2++;
                }
            }
            det += pow(-1.0, j1+2.0) * matrix2[0][j1] * determinant(matTmp, 0);

        }

    }
 /*   if (det == 0) {
           throw std::runtime_error("Function::determinant det ==0");
       }
 */
    return(det);
}


/**
 * @brief determinant_gauss
 * https://codes-sources.commentcamarche.net/source/36387-determinants-gauss-cofacteurs
 * calcul d'un determinant avec le pivot de Gauss,  la complexite est en n^3
 * @param matrix
 * @param shift
 * @return
 */
double determinant_gauss(const Matrix2D &matrix, size_t shift)
{
    size_t n = matrix.size();
    double det;

    if (n - 2*shift == 1) {
          det = matrix[shift][shift];

    } else if (n - 2*shift == 2) {
        det = matrix[shift][shift] * matrix[1+shift][1+shift] - matrix[1+shift][shift] * matrix[shift][1+shift];

    } else {
        Matrix2D matrix2 = seedMatrix(matrix, shift);
        n = matrix2.size();

        Matrix2D matTmp = matrix2; //initLongMatrix(n-1, n-1);

   // size_t   x,y;  // <x> pour les colonnes
                // <y> pour les lignes
    size_t   i,j;  // <i> pour les colonnes
                // <j> pour les lignes

    /* on copie <_mat> dans <mat> car on n'a pas le droit
    * de modifier les valeurs de la matrice <_mat>
    */

 /*   for ( y= 0; y < n; y++)  {
      for( x=0 ;x < n; x++)    {
        matTmp[y][x] = matrix2[y][x];
        }
      }
  */
    /* on calcule le determinant par la methode des pivots de Gauss
    * on rend la matrice triangulaire superieur tout en conservant
    * le determinant de la matrice a chaque iteration
    */
    det = 1.;

    // on balaye les lignes
    for ( j=0; j<n-1; j++)    {

      size_t  rankMax,rank;
      double  coeffMax;

      // ( etape 1 )
      rankMax = j;
      for (rank=j+1;rank<n;rank++) {
        if (fabs(matTmp[rankMax][j]) < fabs(matTmp[rank][j])) {
          rankMax = rank;
        }
      }

      coeffMax = matTmp[rankMax][j];
      if (fabs(coeffMax) <= std::numeric_limits<long double>::epsilon())  {
         return( 0.);
      }
      // ( etape 2 )
      if (rankMax != j)  {
        double tmp;
        for (i=j; i<n; i++) {
          tmp = matTmp[j][i];
          matTmp[j][i] = matTmp[rankMax][i];
          matTmp[rankMax][i] = tmp;
          }
        det *= -1.;
      }

      det *= coeffMax;
      // ( etape 3 )
      double coeff;
      for (rank=j+1; rank<n; rank++) {
        coeff = matTmp[rank][j]/coeffMax;
        for ( i=j; i<n; i++)  {
          matTmp[rank][i] -= coeff*matTmp[j][i];
        }
      }

    }

    det *= matTmp[n-1][n-1];
   }


#ifdef DEBUG
    if (det == 0) {
              throw std::runtime_error("Function::determinant det ==0");
          }
#endif
      //-----
    return (det);
}
// On suppose une matrice carrée N*N
Matrix2D transpose0(const Matrix2D &A)
{
   size_t n = A.size();
   Matrix2D TA  = initMatrix2D(n, n);

   auto Ai = begin(A);
   const double * Aij;
   size_t i, j;
   for ( i = 0 ; Ai != end(A); Ai++, i++)
       for (j= 0, Aij = begin(*Ai) ; Aij != end(*Ai); Aij++, j++)
           TA[j][i] = *Aij ;

   return TA;

}

Matrix2D transpose(const Matrix2D &matrix, const int nbBandes)
{
    const int dim = matrix.size();
    Matrix2D result = initMatrix2D(dim, dim);

    // calcul de la demi-largeur de bande
    const int bande = floor((nbBandes-1)/2);
    int j1;
    int j2;
    for (int i = 0; i < dim; ++i) {
        j1 = std::max(0, i - bande);
        j2 = std::min(dim-1, i + bande);

        for (int j = j1; j <= j2; ++j) {
            result[j][i] = matrix[i][j];
        }
    }
    return result;
}


Matrix2D multiMatParDiag(const Matrix2D& matrix, const std::vector<double>& diag, size_t nbBandes)
{
    const int dim = matrix.size();
    Matrix2D result = initMatrix2D(dim, dim);
    const int bande = floor((nbBandes-1)/2); // calcul de la demi-largeur de bande

    auto matrix_i = matrix[0];
    for (int i = 0; i < dim; ++i) {
        int j1 = std::max(0, i - bande);
        int j2 = std::min(dim-1, i + bande);
       /* if (j1 < 0) {
            j1 = 0;
        }
        if (j2 >= dim) {
            j2 = dim-1;
        }*/

        matrix_i = matrix[i];
        for (int j = j1; j <= j2; ++j) {
            result[i][j] = matrix_i[j] * diag.at(j);
        }
    }
    return result;
}

Matrix2D multiDiagParMat(const std::vector<double>& diag, const Matrix2D& matrix, const int nbBandes)
{
    const int dim = matrix.size();
    Matrix2D result = initMatrix2D(dim, dim);
    const int bande = floor((nbBandes-1)/2); // calcul de la demi-largeur de bande

    for (int i = 0; i < dim; ++i) {
        int j1 = std::max(0, i - bande);
        int j2 = std::min(dim-1, i + bande);
        for (int j = j1; j <= j2; ++j) {
            result[i][j] = diag[i] * matrix[i][j];
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
std::vector<double> multiMatParVec(const Matrix2D& matrix, const std::vector<double>& vec, const int nbBandes)
{
    const int dim = vec.size();
    std::vector<double> result;// (dim);//= initVecteur(dim);
    const int bande = floor((nbBandes-1)/2); // calcul de la demi-largeur de bande
    double sum;
    for (int i = 0; i < dim; ++i) {
        sum = 0.;
        int j1 = std::max(0, i - bande);
        int j2 = std::min(dim-1, i + bande);

        for (int j = j1; j <= j2; ++j) {
            sum += matrix[i][j] * vec[j];
        }
        result.push_back(sum);// result[i] = sum;
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
Matrix2D addMatEtMat0(const Matrix2D& matrix1, const Matrix2D& matrix2)
{
    const int dim = matrix1.size();

    Matrix2D result = matrix1;
    for (int i = 0; i < dim; ++i) {
         for (int j = 0; j < dim; ++j) {
            result[i][j] +=  matrix2[i][j];
        }
    }
    return result;
}

Matrix2D addMatEtMat(const Matrix2D& matrix1, const Matrix2D& matrix2, const int nbBandes2)
{
    const int dim = matrix1.size();
    const int k = floor((nbBandes2-1)/2); // calcul de la demi-largeur de bande

    Matrix2D result = matrix1;
    int j1, j2;
    for (int i = 0; i < dim; ++i) {
        j1 = std::max(0, i - k);
        j2 = std::min(dim-1, i + k);
         for (int j = j1; j <= j2; ++j) {
            result[i][j] +=  matrix2[i][j];
        }
    }
    return result;
}

Matrix2D addIdentityToMat(const Matrix2D& matrix)
{
    const int dim = matrix.size();
    Matrix2D result = matrix;

    for (int i = 0; i < dim; ++i)
        result[i][i] += 1.;

    return result;
}

Matrix2D multiConstParMat(const Matrix2D& matrix, const double c, const int nbBandes)
{
    const int dim = matrix.size();
    Matrix2D result = matrix;
    const int bande = floor((nbBandes-1)/2); // calcul de la demi-largeur de bande

    for (int i = 0; i < dim; ++i) {
        int j1 = std::max(0, i - bande);
        int j2 = std::min(dim-1, i + bande);
        for (int j = j1; j <= j2; ++j) {
            result[i][j] *= c ;
        }
    }
    return result;
}
// without optimization full matrix
Matrix2D multiMatParMat0(const Matrix2D& matrix1, const Matrix2D& matrix2)
{
    const int n = matrix1.size();
    Matrix2D result = initMatrix2D(n, n);
    const double* itMat1;// = begin(matrix1[0]);
    double sum;

    for (int i = 0; i < n; ++i) {
        itMat1 = begin(matrix1[i]);

        for (int j = 0; j < n; ++j) {
           sum = 0;
            for (int k = 0; k < n; ++k) {
                sum += (*(itMat1 + k)) * matrix2[k][j];

            }
            result[i][j] = sum;
        }
    }
    return result;
}


Matrix2D multiMatParMat(const Matrix2D& matrix1, const Matrix2D& matrix2, const int nbBandes1, const int nbBandes2)
{
    const int dim = matrix1.size();
    Matrix2D result = initMatrix2D(dim, dim);

    const int bande1 = floor((nbBandes1-1)/2);
    const int bande2 = floor((nbBandes2-1)/2);
    const int bandeRes = bande1 + bande2;

    for (int i = 0; i < dim; ++i) {
        int j1 = std::max(0, i - bandeRes);
        int j2 = std::min(dim-1, i + bandeRes);
        int k1 = std::max(0, i - bande1);
        int k2 = std::min(dim-1, i + bande1);
        auto itMat1 = begin(matrix1[i]);
        double sum;
        for (int j = j1; j <= j2; ++j) {
            sum = 0;
            for (int k = k1; k <= k2; ++k) {
                sum += (*(itMat1 + k)) * matrix2[k][j];
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
Matrix2D inverseMatSym0(const Matrix2D& matrix, const int shift)
{
    if (matrix.size() != matrix[0].size()) {
           throw std::runtime_error("Matrix is not quadratic");
       }
    Matrix2D matInv = initMatrix2D(matrix.size(), matrix[0].size());

    Matrix2D matrix2 = seedMatrix(matrix, shift);

    int n = matrix.size();
    Matrix2D matInv2 = comatrice0(matrix2);

    const double det = determinant(matrix2);
    if (det == 0) {
           throw std::runtime_error("inverseMatSym0 det == 0");
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
Matrix2D inverseMatSym_origin(const Matrix2D &matrixLE,  const std::vector<double> &matrixDE, const int nbBandes, const int shift)
{
    int dim = matrixLE.size();
    Matrix2D matInv = initMatrix2D(dim, dim);
    int bande = floor((nbBandes-1)/2);

    matInv[dim-1-shift][dim-1-shift] = 1. / matrixDE[dim-1-shift];

    if (dim >= 4) {
        matInv[dim-2-shift][dim-1-shift] = -matrixLE[dim-1-shift][dim-2-shift] * matInv[dim-1-shift][dim-1-shift];
        matInv[dim-2-shift][dim-2-shift] = (1. / matrixDE[dim-2-shift]) - matrixLE[dim-1-shift][dim-2-shift] * matInv[dim-2-shift][dim-1-shift];
    }

    // shift : décalage qui permet d'éliminer les premières et dernières lignes et colonnes
    // La boucle suivante n'est executée que si dim >=5
    for (int i = dim-3-shift; i>=shift; --i) {
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



Matrix2D inverseMatSym(const Matrix2D& matrixLE, const std::vector<double>& matrixDE, const int nbBandes, const int shift)
{
    const int dim = matrixLE.size();
    Matrix2D matInv = initMatrix2D(dim, dim);
    const int bande = floor((nbBandes-1)/2);

    matInv[dim-1-shift][dim-1-shift] = 1. / matrixDE.at(dim-1-shift);
    matInv[dim-2-shift][dim-1-shift] = -matrixLE[dim-1-shift][dim-2-shift] * matInv[dim-1-shift][dim-1-shift];
    matInv[dim-2-shift][dim-2-shift] = (1. / matrixDE.at(dim-2-shift)) - matrixLE[dim-1-shift][dim-2-shift] * matInv[dim-2-shift][dim-1-shift];

    if (bande >= 3) {
        for (int i = dim-3-shift; i >= shift; --i) {
                matInv[i][i+2] = -matrixLE[i+1][i] * matInv[i+1][i+2] - matrixLE[i+2][i] * matInv[i+2][i+2];
                matInv[i][i+1] = -matrixLE[i+1][i] * matInv[i+1][i+1] - matrixLE[i+2][i] * matInv[i+1][i+2];
                matInv[i][i] = (1. / matrixDE[i]) - matrixLE[i+1][i] * matInv[i][i+1] - matrixLE[i+2][i] * matInv[i][i+1];

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
            matInv[i][i] = (1. / matrixDE[i]) - matrixLE[i+1][i] * matInv[i][i+1] - matrixLE[i+2][i] * matInv[i][i+2];

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

Matrix2D cofactor0(const Matrix2D& matrix)
{
    const int n = matrix.size();
    Matrix2D result = initMatrix2D(n, n);
    Matrix2D matMinorTmp = initMatrix2D(n-1, n-1);
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
                    matMinorTmp[i1][k1] = matrix[k][l];
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
Matrix2D comatrice0(const Matrix2D& matrix)
{
    const int n = matrix.size();
    Matrix2D result = initMatrix2D(n, n);
    Matrix2D matMinorTmp = initMatrix2D(n-1, n-1);
    double det;
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
                    matMinorTmp[i1][k1] = matrix[k][l];
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

Matrix2D choleskyLL0(const Matrix2D &matrix)
{
    const int n = matrix.size();

    Matrix2D L = initMatrix2D(n, n);
    double sum;

    for (int i=0; i<n; i++) {
        sum = matrix[i][i];
        for (int k=0; k<i; k++)
            sum -= pow(L[i][k], 2.);

        L[i][i] = sqrt(sum);

        for (int j=i+1; j<n; j++) {
            sum = matrix[i][j];

            for (int k=0; k<j-1; k++)
                sum -=  L[j][k] * L[i][k];

            L[j][i] = sum / L[i][i];
        }
    }

    return L;
}

/**
 * @brief choleskyLDL
 * @param matrix
 * @return pair of 2 matrix
 */
std::pair<Matrix2D, std::vector<double> > choleskyLDLT(const Matrix2D& matrix)
{
    // fonction à controler
    const int n = matrix.size();

    Matrix2D L = initMatrix2D(n, n);
    std::vector<double> D = initVector(n);

    for (int i=0; i<n; i++) {
            L[i][i] = 1;
            for (int j=0; j<i; j++) {
                L[i][j] = matrix[i][j];
                for (int k=0; k<j; k++) {
                   L[i][j] -=  L[i][k] * L[j][k] *D.at(k);
                }
                L[i][j] /= D.at(j);
            }
            D[i] = matrix[i][i];
            for (int j=0; j<i; j++)
                D[i] -=  D.at(j) * pow(L[i][j], 2.);
      }

    return std::pair<Matrix2D, std::vector< double>>(L, D);
}

std::pair<Matrix2D, std::vector<double> > choleskyLDLT(const Matrix2D& matrix, const int shift)
{
    const int n = matrix.size();

    Matrix2D L = initMatrix2D(n, n);
    std::vector<double> D (n);

    for (int i = shift; i < n-shift; i++) {
            L[i][i] = 1;
            for (int j = shift; j < std::min(i, n-shift); j++) {
                L[i][j] = matrix[i][j];
                for (int k = shift; k< std::min(j, n-shift); k++) {
                   L[i][j] -=  L[i][k] * L[j][k] *D.at(k);
                }
                L[i][j] /= D[j];
            }
            D[i] = matrix[i][i];
            for (int j = shift; j< std::min(i, n-shift); j++)
                D[i] -=  D[j] * pow(L[i][j], 2.);
      }

    return std::pair<Matrix2D, std::vector< double>>(L, D);
}

std::pair<Matrix2D, std::vector<double> > choleskyLDLT(const Matrix2D& matrix, const int nbBandes, const int shift)
{
    // fonction à controler
    const int n = matrix.size();

    Matrix2D L = initMatrix2D(n, n);
    std::vector<double> D (n);

    for (int i = shift; i < n-shift; i++) {
            L[i][i] = 1;
            for (int j = shift; j < std::min(i, n-shift); j++) {
               if (abs(i - j) <= nbBandes) {
                   L[i][j] = matrix[i][j];
                   for (int k = shift; k< std::min(j, n-shift); k++) {
                       L[i][j] -=  L[i][k] * L[j][k] *D.at(k);
                   }
                   L[i][j] /= D[j];
               }
            }
            D[i] = matrix[i][i];
            for (int j = shift; j< std::min(i, n-shift); j++)
                if (abs(i - j) <= nbBandes)
                    D[i] -=  D[j] * pow(L[i][j], 2.);
      }

    return std::pair<Matrix2D, std::vector< double>>(L, D);
}


std::pair<Matrix2D, std::vector<double> > choleskyLDLT_Dsup0(const Matrix2D& matrix, const int nbBandes, const int shift)
{
    // fonction à controler
    const int n = matrix.size();

    Matrix2D L = initMatrix2D(n, n);
    std::vector<double> D = initVector(n);

    for (int i = shift; i < n-shift; i++) {
            L[i][i] = 1;
            for (int j = shift; j < std::min(i, n-shift); j++) {
               if (abs(i - j) <= nbBandes) {
                   L[i][j] = matrix[i][j];
                   for (int k = shift; k< std::min(j, n-shift); k++) {
                       L[i][j] -=  L[i][k] * L[j][k] *D.at(k);
                   }
                   L[i][j] /= D[j];
               }
            }
            D[i] = matrix[i][i];
            for (int j = shift; j< std::min(i, n-shift); j++) {
                if (abs(i - j) <= nbBandes)
                    D[i] -=  D[j] * pow(L[i][j], 2.);
            }

      }

    for (int i = shift; i < n-shift; i++) {
        if (D[i] < 0) {
            qDebug() << "Function::choleskyLDLT_Dsup0 : D <0 change to 0"<< D[i];
            D[i] = 0;
        }
    }
    return std::pair<Matrix2D, std::vector< double>>(L, D);
}

/** ****************************************************************************
**** Cette procedure effectue la decomposition de Cholesky                 ****
**** sur la matrice passe en parametre (para)                              ****
**** Cf algorithme donné par S.M.Kay, "Modern Spectral Analysis"           ****
**** 1988, page 30.                                                        ****
**** Cette décomposition ne peut s'appliquer qu'à des matrices symétriques ****
*******************************************************************************/

//  link to check  https://mxncalc.com/fr/cholesky-decomposition-calculator
std::pair<Matrix2D, std::vector<double>> decompositionCholesky(const Matrix2D& matrix, const int nbBandes, const int shift)
{
    errno = 0;
      //if (math_errhandling & MATH_ERREXCEPT) feclearexcept(FE_ALL_EXCEPT);

    const int dim = matrix.size();
    Matrix2D matL = initMatrix2D(dim, dim);
    std::vector< double> matD (dim);

    if (dim - 2*shift == 1) { // cas des splines avec 3 points
       /* long double Wh1_1 = matrix[1][0];
        long double Wh2_1 = matrix[1][1];
        long double Wh3_1 = matrix[2][0];

        const long double b = pow(Wh1_1, 2) + pow(Wh3_1, 2) + pow(Wh2_1 ,2.);
        */
        matD[1] = matrix[1][1];;
        matL[1][1] = 1.;

    } else {

        // const int bande = floor((nbBandes-1)/2);

        // shift : décalage qui permet d'éliminer les premières et dernières lignes et colonnes constituées de zéro
        for (int i = shift; i < dim-shift; ++i) {
            matL[i][i] = 1.;
        }
        matD[shift] = matrix[shift][shift];

        try {
            for (int i = shift+1; i < dim-shift; ++i) {
                matL[i][shift] = matrix[i][shift] / matD[shift];
                /*   avec bande */
                for (int j = shift+1; j < i; ++j) {
                    if (abs(i - j) <= nbBandes) {
                        double sum = 0.;
                        for (int k = shift; k < j; ++k) {
                            if (abs(i - k) <= nbBandes) {
                                sum += matL[i][k] * matD.at(k) * matL[j][k];
                            }
                        }
                        matL[i][j] = (matrix[i][j] - sum) / matD.at(j);
                    }
                }

                double sum = 0.;
                for (int k = shift; k < i; ++k) {
                    if (abs(i - k) <= nbBandes) {
                        sum += std::pow(matL[i][k], 2.) * matD[k];
                    }
                }


                matD[i] = matrix[i][i] - sum; // doit être positif

            }

            for (int i = shift; i < dim-shift; ++i) {
                if (matD[i] < 0) {
                    qDebug() << "Function::decompositionCholesky : matD <0 change to 0"<< matD[i];
                    matD[i] = 0;
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

        } catch(...) {
            qDebug() << "Function::decompositionCholesky : Caught Exception!\n";
        }
    }

   std::pair<Matrix2D, std::vector<double>> tmp =  choleskyLDLT_Dsup0(matrix, nbBandes, shift);
    return std::pair<Matrix2D, std::vector<double>>(matL, matD);
}

std::vector<double> resolutionSystemeLineaireCholesky(const Matrix2D &matL, const std::vector<double> &matD, const std::vector<double> &vecQtY)
{
    const int n = matD.size(); //mModel->mEvents.size(); // pHd :: ?? est-ce que matD à la même dimension que mEvent?? -> NON
    std::vector<double> vecGamma (n);
    std::vector<double> vecU (n);
    std::vector<double> vecNu (n);

    if (n > 3 ) {
        vecU[1] = vecQtY[1];
        vecU[2] = vecQtY[2] - matL[2][1] * vecU[1];

        for (int i = 3; i < n-1; ++i) {
            vecU[i] = vecQtY[i] - matL[i][i-1] * vecU[i-1] - matL[i][i-2] * vecU[i-2]; // pHd : Attention utilisation des variables déjà modifiées
        }

        for (int i = 1; i < n-1; ++i) {
            vecNu[i] = vecU[i] / matD[i];
        }

        vecGamma[n-2] = vecNu.at(n-2);
        vecGamma[n-3] = vecNu.at(n-3) - matL[n-2][n-3] * vecGamma[n-2];

        for (int i = n-4; i > 0; --i) {
            vecGamma[i] = vecNu[i] - matL[i+1][i] * vecGamma[i+1] - matL[i+2][i] * vecGamma[i+2]; // pHd : Attention utilisation des variables déjà modifiées
        }

    } else {
        // cas n = 3
        vecGamma[1] = vecQtY[1] / matD[1];
    }

    return vecGamma;
}


/**
 * @brief Strassen::sub substraction of 2 square matrix NxN
 * @param A square matrix NxN
 * @param B square matrix NxN
 * @return one square matrix NxN
 */
Matrix2D Strassen::sub(const Matrix2D &  A, const Matrix2D& B)
{
    const int n = A.size();

    Matrix2D C  = initMatrix2D(n, n);

    std::valarray< double>* ci = begin(C);
    double* cij;

    const std::valarray<double>* ai = begin(A);
    const std::valarray<double>* bi = begin(B);
    const double* aij;// = begin(*ai);
    const double* bij;// = begin(*bi);

    for ( ; ai != end(A); ++ai, ++bi) {
        cij = begin(*ci);
        for (aij = begin(*ai), bij = begin(*bi) ; aij != end(*ai); ++aij, ++bij) {
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
Matrix2D Strassen::add(const Matrix2D& A, const Matrix2D& B)
{
    const int n = A.size();

   Matrix2D C = initMatrix2D(n, n);

   std::valarray<double>* ci = begin(C);
   double* cij;

   const std::valarray<double>* ai = begin(A);
   const std::valarray<double>* bi = begin(B);
   const double* aij;// = begin(*ai);
   const double* bij;// = begin(*bi);

   for ( ; ai != end(A); ++ai, ++bi) {
       cij = begin(*ci);
       for (aij = begin(*ai), bij = begin(*bi) ; aij != end(*ai); ++aij, ++bij) {
           *cij = *aij + *bij;
           ++cij;
       }
       ++ci;
   }
   return C;
}
/** Funtion to split parent matrix into child matrices **/

void Strassen::split(const Matrix2D& P,  Matrix2D& C, int iB, int jB)
{
    const int lig = C.size();
    const int col = C[0].size();

    for (int i1 = 0, i2 = iB; i1 < lig; ++i1, ++i2)
        for (int j1 = 0, j2 = jB; j1 < col; ++j1, ++j2)
            C[i1][j1] = P[i2][j2];

}
void Strassen::join(const Matrix2D& C,  Matrix2D& P, int iB, int jB)
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
Matrix2D Strassen::multiply(const Matrix2D& A, const Matrix2D& B)
{
    const int n = A.size();
    Matrix2D R;

    /** base case **/

    if (n == 1) {
        R = initMatrix2D(1, 1);
        R[0][0] = A[0][0] * B[0][0];

     } else {
        // controle de la puissance de n, n doit être une puissance de 2
        int nP2;

        Matrix2D A2;
        Matrix2D B2;

        if ( n != pow( 2., floor(log(n)/log(2))) ) { //null row and column filling
            nP2 = pow( 2., floor(log(n)/log(2)) + 1);
            A2 = initMatrix2D(nP2, nP2);
            B2 = initMatrix2D(nP2, nP2);

            join (A, A2, 0, 0);
            join (B, B2, 0, 0);

        } else {
            A2 = A;
            B2 = B;
            nP2 = n;
        }

        const int n1 = floor(nP2/2);

        Matrix2D A11 = initMatrix2D(n1, n1);// (n1, std::vector<long double>(n1, 0.));
        Matrix2D A12 = initMatrix2D(n1, n1);//(n1, std::vector<long double>(n1, 0.));
        Matrix2D A21 = initMatrix2D(n1, n1);//(n1, std::vector<long double>(n1, 0.));
        Matrix2D A22 = initMatrix2D(n1, n1);//(n1, std::vector<long double>(n1, 0.));

        Matrix2D B11 = initMatrix2D(n1, n1);//(n1, std::vector<long double>(n1, 0.));
        Matrix2D B12 = initMatrix2D(n1, n1);//(n1, std::vector<long double>(n1, 0.));
        Matrix2D B21 = initMatrix2D(n1, n1);//(n1, std::vector<long double>(n1, 0.));
        Matrix2D B22 = initMatrix2D(n1, n1);//(n1, std::vector<long double>(n1, 0.));



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



        Matrix2D M1 = multiply(add(A11, A22), add(B11, B22));

        Matrix2D M2 = multiply(add(A21, A22), B11);

        Matrix2D M3 = multiply(A11, sub(B12, B22));

        Matrix2D M4 = multiply(A22, sub(B21, B11));

        Matrix2D M5 = multiply(add(A11, A12), B22);

        Matrix2D M6 = multiply(sub(A21, A11), add(B11, B12));

        Matrix2D M7 = multiply(sub(A12, A22), add(B21, B22));



        /**

          C11 = M1 + M4 - M5 + M7

          C12 = M3 + M5

          C21 = M2 + M4

          C22 = M1 - M2 + M3 + M6

        **/

        Matrix2D C11 = add(sub(add(M1, M4), M5), M7);
        Matrix2D C12 = add(M3, M5);
        Matrix2D C21 = add(M2, M4);
        Matrix2D C22 = add(sub(add(M1, M3), M2), M6);

        R = initMatrix2D(nP2, nP2);

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
std::pair<Matrix2D, Matrix2D > decompositionLU0(const Matrix2D& A)
{
   const unsigned n = A.size();

    Matrix2D L = initMatrix2D(n, n);
    Matrix2D U = initMatrix2D(n, n);

    unsigned i, j , k;

    for (i = 0; i < n; i++) {
        // Upper triangle
        for (k  = i; k < n; k++) {
            U[i][k] = A[i][k];
            for (j = 0; j < i; j++)
                U[i][k] -=  U[j][k] * L[i][j];
        }

        // diagonal creation
        L[i][i] = 1.;

        // Lower triangle
        for (k = i+1; k < n; k++) {
            L[k][i] = A[k][i];
            for (j = 0; j < i; j++)
                L[k][i] -=  U[j][i] * L[k][j];

            L[k][i] /= U[i][i];
        }


    }

    return std::pair<Matrix2D, Matrix2D>(L, U);
}
