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

#include "Functions.h"
#include "Generator.h"
#include "QtUtilities.h"
#include "AppSettings.h"
#include "StdUtilities.h"

#include "fftw3.h"

#include <QDebug>
#include <QApplication>
#include <QThread>

#include <iostream>
#include <map>
#include <QTime>
#include <QElapsedTimer>
//#include <experimental/algorithm>

#ifdef _OPENMP
#include <omp.h>
#endif

#include <cmath>  // pour expl, powl, std::isnan
#include <errno.h>      /* errno, EDOM */

// -----------------------------------------------------------------
//  sumP = Sum (pi)
//  sum = Sum (pi * xi)
//  sum2 = Sum (pi * xi^2)
// -----------------------------------------------------------------

/**
 * @brief Product a FunctionStat from a QMap
 * @todo Handle empty function case and null density case (pi = 0)
 *
 *  Calculate the variance based on equations (15) and (16) on
 *  page 216 of "The Art of Computer Programming, Volume 2",
 *  Second Edition. Donald E. Knuth.  Addison-Wesley
 *  Publishing Company, 1973.
*/

FunctionStat analyseFunction(const QMap<type_data, type_data> &fun)
{
    FunctionStat result;
    if (fun.isEmpty()) {
        result.max = (type_data)0.;
        result.mode = (type_data)0.;
        result.mean = (type_data)0.;
        result.std = (type_data)(-1.);
        qDebug() << "[Function::analyseFunction] WARNING : No data !!";
        return result;

    } else if (fun.size() == 1) {
        result.max = fun.firstKey();
        result.mode = fun.firstKey();
        result.mean = fun.firstKey();
        result.std = 0.;
        //qDebug() << "[Function::analyseFunction] WARNING : Only one data !! ";
        result.quartiles.Q1 = fun.firstKey();
        result.quartiles.Q2 = fun.firstKey();
        result.quartiles.Q3 = fun.firstKey();
        return result;
    }

    type_data max (0.);
    type_data mode (0.);

    type_data prev_y (0.);
    QList<type_data> uniformXValues;

    QMap<type_data,type_data>::const_iterator citer = fun.cbegin();

    type_data y_sum = 0;
    type_data mean = 0;

    type_data variance = 0;

    // Suppression des premiers zéro qui genent le calcul de moyenne

    while (citer != fun.cend() && (citer.value()) == 0) {
        ++citer;
    }

    for (;citer != fun.cend(); ++citer) {

        const type_data x = citer.key();
        const type_data y = citer.value();
        y_sum +=  y;

        const type_data mean_prev = mean;
        mean +=  (y / y_sum) * (x - mean_prev);
        variance += y * (x - mean_prev) * (x - mean);


        // Find max
        if (max <= y) {
            max = y;
            // Tray detection
            if (prev_y == y) {
                if (uniformXValues.isEmpty()) {
                    uniformXValues.append(std::prev(citer).key()); // Adding prev_x
                }
                uniformXValues.append(x);
               // Arbitrary the mode is in the middle of the tray
                mode = (uniformXValues.first() + uniformXValues.last())/2.;

            } else {
                uniformXValues.clear(); // If necessary, end of tray
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

    const double step = (fun.lastKey() - fun.firstKey()) / static_cast<double>(fun.size() - 1);
    QList<double> subRepart = calculRepartition(fun);
    result.quartiles = quartilesForRepartition(subRepart, fun.firstKey(), step);

    return result;
}

FunctionStat analyseFunction(const std::map<type_data, type_data> &fun)
{
    FunctionStat result;
    if (fun.empty()) {
        result.max = (type_data)0.;
        result.mode = (type_data)0.;
        result.mean = (type_data)0.;
        result.std = (type_data)(-1.);
        qDebug() << "[Function::analyseFunction] WARNING : No data !!";
        return result;

    } else if (fun.size() == 1) {
        result.max = fun.begin()->first;
        result.mode = fun.begin()->first;
        result.mean = fun.begin()->first;
        result.std = 0.;
        //qDebug() << "[Function::analyseFunction] WARNING : only one data !! ";
        result.quartiles.Q1 = fun.begin()->first;
        result.quartiles.Q2 = fun.begin()->first;
        result.quartiles.Q3 = fun.begin()->first;
        return result;
    }

    type_data max (0.);
    type_data mode (0.);

    type_data prev_y (0.);
    QList<type_data> uniformXValues;

    std::map<type_data,type_data>::const_iterator citer = fun.cbegin();

    type_data y_sum = 0;
    type_data mean = 0;
    type_data mean_prev;
    type_data x;
    type_data y = citer->second;
    type_data variance = 0;

    // suppression des premiers zéro qui genent le calcul de moyenne
    for (;citer != fun.cend() && y == 0; ++citer) {
        y = citer->second;
    }

    for (;citer != fun.cend(); ++citer) {

        x = citer->first;
        y = citer->second;
        y_sum +=  y;

        mean_prev = mean;
        mean +=  (y / y_sum) * (x - mean_prev);
        variance += y * (x - mean_prev) * (x - mean);

        // Find max
        if (max <= y) {
            max = y;
            // Tray detection
            if (prev_y == y) {
                if (uniformXValues.isEmpty()) {
                    uniformXValues.append(std::prev(citer)->first); // Adding prev_x
                }
                uniformXValues.append(x);
                // Arbitrary the mode is in the middle of the tray
                mode = (uniformXValues.first() + uniformXValues.last())/2.;

            } else {
                uniformXValues.clear(); // If necessary, end of tray
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

    const double step = (fun.crbegin()->first - fun.begin()->first) / static_cast<double>(fun.size() - 1);
    std::vector<double> subRepart = calculRepartition(fun);
    result.quartiles = quartilesForRepartition(subRepart, fun.begin()->first, step);

    return result;
}
/**
 * @brief std_Koening Algorithm using the Koenig-Huygens formula, can induce a negative variance
 * return std biaised, must be corrected if unbiais needed
 * @param data
 * @return
 */
type_data std_Koening(const QList<type_data> &data)
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
double variance_Knuth(const QList<double> &data)
{
    unsigned n = 0;
    double mean = 0.0;
    double variance = 0.0;

    for (const auto& x : data) {
        n++;
        double previousMean = mean;
        mean += (x - previousMean)/n;
        variance +=  (x - previousMean)*(x - mean);
    }

    return variance/static_cast<double>(n);

}

double variance_Knuth(const std::vector<double> &data)
{
    unsigned n = 0;
    double mean = 0.0;
    double variance = 0.0;

    for (const auto& x : data) {
        n++;
        double previousMean = mean;
        mean += (x - previousMean)/n;
        variance +=  (x - previousMean)*(x - mean);
    }

    return variance/static_cast<double>(n);

}

double variance_Knuth(const std::vector<t_matrix> &data)
{
    unsigned n = 0;
    t_matrix mean = 0.0;
    t_matrix variance = 0.0;

    for (const auto& x : data) {
        n++;
        t_matrix previousMean = mean;
        mean += (x - previousMean)/n;
        variance +=  (x - previousMean)*(x - mean);
    }

    return variance/static_cast<double>(n);

}

double variance_Knuth(const std::vector<int> &data)
{
    int n = 0;
    double mean = 0.;
    double variance = 0.;

    for (const auto& x : data) {
        n++;
        double x_double = static_cast<double>(x); // Conversion unique
        double previousMean = mean;
        mean +=  (x_double - previousMean) / n;
        variance +=  ( x_double - previousMean)*( x_double - mean);
    }

    return variance/static_cast<double>(n);

}

void mean_variance_Knuth(const std::vector<double> &data, double &mean, double &variance)
{
    int n = 0;
    variance = 0.0;
    mean = 0.0;

    for (const auto& x : data) {
        n++;
        double previousMean = mean;
        mean += (x - previousMean) / n;
        variance +=  ( x - previousMean)*( x - mean);
    }
    variance /= n;
}

void mean_variance_Knuth(const QList<double> &data, double& mean, double& variance)
{
    int n = 0;
    variance = 0.0;
    mean = 0.0;

    for (auto&& x : data) {
        n++;
        double previousMean = mean;
        mean += (x - previousMean) / n;
        variance += ( x - previousMean)*( x - mean);
    }
    variance /= n;
}

double std_unbiais_Knuth (const QList<double> &data)
{
    unsigned n = 0;
    double mean = 0.0;
    double variance = 0.0;

    for (auto&& x : data) {
        n++;
        double previousMean = mean;
        mean += (x - previousMean)/n;
        variance +=  (x - previousMean)*(x - mean);
    }

    return sqrt(variance/static_cast<double>(n-1)); // unbiais

}

double std_unbiais_Knuth(const std::vector<double> &data)
{
    unsigned n = 0;
    double mean = 0.;
    double variance = 0.;

    for (auto&& x : data) {
        n++;
        double previousMean = mean;
        mean += (x - previousMean)/n;
        variance += (x - previousMean)*(x - mean);
    }

    return sqrt(variance/static_cast<long double>(n-1));

}

double std_unbiais_Knuth(const std::vector<int>& data)
{
    int n = 0;
    double mean = 0.0;
    double variance = 0.0;

    for (const auto& x : data) {
        n++;
        double x_double = static_cast<double>(x); // Conversion unique
        double previousMean = mean;
        mean += (x_double - previousMean) / n;
        variance +=  ( x_double - previousMean)*( x_double - mean);
    }

    return sqrt(variance/(n-1)); // unbiais

}

void mean_std_unbiais_Knuth(const std::vector<int>& data, double& mean, double& std)
{
    int n = 0;
    mean = 0.0;
    double variance = 0.0;

    for (const auto& x : data) {
        n++;
        double x_double = static_cast<double>(x); // Conversion unique
        double previousMean = mean;
        mean += (x_double - previousMean) / n;
        variance +=  ( x_double - previousMean)*( x_double - mean);
    }

    std = sqrt(variance/(n-1));// unbiais

}

// dataX and dataY must have the same length
double covariance(const std::vector<double> &dataX, const std::vector<double> &dataY)
{
    double meanx = 0.0;
    double meany = 0.0;
    double covar = 0.0;
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

QList<double> autocorrelation_schoolbook(const QList<double> &trace, const int hmax)
{
    QList<double> results;
    const auto n = trace.size();

    double mean, variance;
    mean_variance_Knuth(trace, mean, variance);
    variance *= (double)trace.size();

    results.append(1.); // force the first to exactly 1.
    double sH = 0.;
    for (int h = 1; h <= hmax; ++h) {
        sH = 0.;
        QList<double>::const_iterator iter_H = trace.cbegin() + h;
        for (QList<double>::const_iterator iter = trace.cbegin(); iter != trace.cbegin() + (n-h); ++iter)
            sH += (*iter - mean) * (*iter_H++ - mean);

        results.append(sH / variance);
    }

    return results;

}


/**
 * @brief autocorrelation_schoolbook,
 * Si hmax ⁡≪ n, Alors ta méthode « schoolbook » optimisée est plus rapide que la FFT (setup FFT + padding + overhead).
 * @param trace
 * @param hmax
 * @return
 */
std::vector<double> autocorrelation_schoolbook(const std::vector<double> &trace, const int hmax)
{

    const size_t n = trace.size();

    double mean, variance;
    mean_variance_Knuth(trace, mean, variance);
    variance *= static_cast<double>(n);

    /*
     std::vector<double> results;
    results.push_back(1.); // force the first to exactly 1.
    double sH = 0.;
    for (int h = 1; h <= hmax; ++h) {
        sH = 0.;
        std::vector<double>::const_iterator iter_H = trace.cbegin() + h;
        for (std::vector<double>::const_iterator iter = trace.cbegin(); iter != trace.cbegin() + (n-h); ++iter)
            sH += (*iter - mean) * (*iter_H++ - mean);

        results.push_back(sH / variance);
    }
    */
    // Optimisation, moins d’accès mémoire et pas de reallocations
    std::vector<double> results(hmax + 1);
    // pré-centrer la série
    std::vector<double> centered(n);
    for (size_t i = 0; i < n; ++i)
        centered[i] = trace[i] - mean;

    results[0] = 1.0; // corr(0) = 1
    for (int h = 1; h <= hmax; ++h) {
        double sH = 0.0;
        for (size_t i = 0; i < n - h; ++i)
            sH += centered[i] * centered[i + h];
        results[h] = sH / variance;
    }
    return results;

}


QList<double> autocorrelation_by_convol(const QList<double> &trace, const int hmax)
{

    double mean, variance;
    mean_variance_Knuth(trace, mean, variance);
    const int inputSize = (int) trace.size();

    const int paddingSize = ceil(inputSize/2) + 1; // Doit être supérieur à inptSize/2, sinon chevauchement des traces à convoler

    const int N = inputSize + 2*paddingSize;
    const int NComplex = 2* (N/2)+1;

    double *inputReal;
    inputReal = new double [N];
    fftw_complex *inputComplex;
    inputComplex = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * NComplex);

    // we could use std::copy

    for (int i  = 0; i< paddingSize; i++) {
        inputReal[i] = 0;
    }
    for (int i = 0; i< inputSize; i++) {
        inputReal[i+paddingSize] = trace[i] - mean;
    }
    for (int i = inputSize+paddingSize; i< N; i++) {
        inputReal[i] = 0;
    }

    fftw_plan plan_input = fftw_plan_dft_r2c_1d(N, inputReal, inputComplex, FFTW_ESTIMATE);
    fftw_execute(plan_input);

    fftw_complex *outputComplex;
    outputComplex = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * NComplex);
    //
    // FORMULE convolution g o f
    //    for (int i= 1; i<NComplex; ++i) {
    //        outputComplex[i][0] = gComplex[i][0] * fComplex[i][0] - gComplex[i][1] * fComplex[i][1];
    //        outputComplex[i][1] = gComplex[i][0] * fComplex[i][1] + gComplex[i][1] * fComplex[i][0];
    //    }

    // remarque : mean = sqrt(inputComplex[0][0]^2 + inputComplex[0][1]^2)/inputSize
    // ici img(inputComplex)= inputComplex[0][1] = 0 donc mean = real(inputComplex)/inputSize = inputComplex[0][0]/inputSize
    // or ici nous avons fait la différence entre les valeurs de t et mean_t (input=t-mean) donc la valeur moyenne vaut zéro
    //qDebug()<<" inputComplex0="<<sqrt(inputComplex[0][0] * inputComplex[0][0] + inputComplex[0][1] * inputComplex[0][1])/inputSize<<inputComplex[0][0]/inputSize << inputComplex[0][1];


    //inputComplex[0][0]=0;
    //inputT_Complex[0][0]=0;
    // FORMULE convolution de l'auto-correlation g o g*
    // Ici il s'agit de la formule pour convoler g par son conjugué complexe g*
    // si A = a+ib  son conjugué B = A* = a-ib avec a=real(A) et b=img(A)

    for (int i= 0; i<NComplex; ++i) {
        outputComplex[i][0] = pow(inputComplex[i][0], 2.) + pow(inputComplex[i][1], 2.);
        outputComplex[i][1] = - inputComplex[i][0] * inputComplex[i][1] + inputComplex[i][1] * inputComplex[i][0];
    }

    double *outputReal;
    outputReal = new double [N];
    fftw_plan plan_output = fftw_plan_dft_c2r_1d(N, outputComplex, outputReal, FFTW_ESTIMATE);

    fftw_execute(plan_output);

    const double vmax = outputReal[0];
    // La sortie de la convolution= autro-correlation donne /
    // outputReal[0]=somme(input^2)
    // comme nous avons fait input=t-mean; outputReal[0] vmax = somme((t-mean)^2) = variance* N
    QList<double> results;
    for ( int i = 0; i < hmax + 1; i++) {
        results.push_back(outputReal[i] /vmax);
    }
    fftw_destroy_plan(plan_input);
    fftw_destroy_plan(plan_output);

    fftw_free(inputComplex);
    fftw_free(outputComplex);

    delete [] inputReal;
    delete [] outputReal;

    fftw_cleanup();
    return results;
}

/**
 * @brief linear_regression using knuth algorithm
 * @param dataX
 * @param dataY
 * @return a, b coef and constant $$Y = a*t + b$$
 * @remark dataX and dataY must have the same length
 */
const std::pair<double, double> linear_regression(const std::vector<double> &dataX, const std::vector<double>& dataY)
{
    double mean_x = 0.0;
    double mean_y = 0.0;
    double covar = 0.0;
    double var_x = 0.0;
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

#pragma mark TRACE STAT
/**
 * @brief traceStatistic : This function uses the Knuth-Welford algorithm to calculate the standard deviation.
 * @param trace
 * @return
 */
TraceStat traceStatistic(const QList<type_data> &trace)
{
    TraceStat result;
    if (trace.size() == 1) {
        result.mean = trace.at(0);
        result.std = 0.0; // unbiais

        result.min = trace.at(0);
        result.max = trace.at(0);

        result.quartiles.Q1 = trace.at(0);
        result.quartiles.Q2 = trace.at(0);
        result.quartiles.Q3 = trace.at(0);

        return result;
    }

    int n = 0;
    type_data mean = 0.0;
    type_data variance = 0.0;

    for (const auto& x : trace) {
        n++;
        double previousMean = mean;
        mean += (x - previousMean)/n;
        variance += (x - previousMean)*(x - mean);
    }
    result.mean = mean;
    result.std = sqrt(variance/(n-1)); // unbiais

    auto minMax = std::minmax_element(trace.begin(), trace.end());
    result.min = std::move(*minMax.first);
    result.max = std::move(*minMax.second);

    result.quartiles = quartilesForTrace(trace);
    return result;
}

TraceStat traceStatistic(const std::vector<type_data> &trace)
{
    TraceStat result;
    if (trace.size() == 1) {
        result.mean = trace.at(0);
        result.std = 0.0; // unbiais

        result.min = trace.at(0);
        result.max = trace.at(0);

        result.quartiles.Q1 = trace.at(0);
        result.quartiles.Q2 = trace.at(0);
        result.quartiles.Q3 = trace.at(0);

        return result;
    }

    int n = 0;
    type_data mean = 0.0;
    type_data variance = 0.0;

    for (auto&& x : trace) {
        n++;
        type_data previousMean = mean;
        mean +=  (x - previousMean)/n;
        variance += (x - previousMean)*(x - mean);
    }
    result.mean = mean;
    result.std = sqrt(variance/(n-1)); // unbiais

    auto minMax = std::minmax_element(trace.begin(), trace.end());
    result.min = *minMax.first;
    result.max = *minMax.second;

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
QString FunctionStatToString(const FunctionStat &analysis)
{
    QString result;

    if (analysis.std<0.)
        result = QObject::tr("No data");

    else {
        result += QObject::tr("MAP = %1  ;  Mean = %2  ;  Std = %3").arg( stringForLocal(analysis.mode),
                                                                         stringForLocal(analysis.mean),
                                                                         stringForLocal(analysis.std)) + "<br>";
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
QString densityAnalysisToString(const DensityAnalysis &analysis)
{
    QString result (QObject::tr("No data"));
    if (analysis.funcAnalysis.std >= 0.) {

        result = "<i>" + QObject::tr("Trace Stat.")  + "</i><br>";
        result += QObject::tr("Mean = %1  ;  Std = %2").arg( stringForLocal(analysis.traceAnalysis.mean),
                                                            stringForLocal(analysis.traceAnalysis.std)) + "<br>";
        result += QObject::tr("Q1 = %1  ;  Q2 (Median) = %2  ;  Q3 = %3 ").arg( stringForLocal(analysis.traceAnalysis.quartiles.Q1),
                                                                               stringForLocal(analysis.traceAnalysis.quartiles.Q2),
                                                                               stringForLocal(analysis.traceAnalysis.quartiles.Q3)) + "<br>";
        result += QObject::tr("min = %1  ;  max  = %2 ").arg( stringForLocal(analysis.traceAnalysis.min),
                                                             stringForLocal(analysis.traceAnalysis.max)) + "<br>";
        result += "<br><i>" + QObject::tr("Density Stat.") + "</i><br>";
    }

    result += FunctionStatToString(analysis.funcAnalysis) + "<br>";


    return result;
}


Quartiles quartilesForTrace(const QList<type_data> &trace)
{
    Quartiles quartiles = quantilesType(trace, 8, 0.25);
    return quartiles;
}

Quartiles quartilesForTrace(const std::vector<type_data> &trace)
{
    Quartiles quartiles = quantilesType(QList<type_data>(trace.begin(), trace.end()), 8, 0.25);
    return quartiles;
}

/*
QList<double> calculRepartition(const QList<double> &calib)
{
    QList<double> repartitionTemp;

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
*/
/*
QList<double> calculRepartition(const QMap<double, double>  &calib)
{
    QList<double> repartitionTemp;

    // we use long double type because
    // after several sums, the repartion can be in the double type range
    long double lastV = 0;
    QMap<double, double>::const_iterator it (calib.cbegin());

    //long double lastRepVal (lastV);
    long double rep = 0.;


    for (auto [key, value] : calib.asKeyValueRange()) {
        //const long double v = it.value();

        if(value != 0. && lastV != 0.)
            //rep = lastRepVal + (t-lastT)*(lastV + v) / 2.l;
            // rep = lastRepVal + (lastV + v); // step is constant
            rep +=  (lastV + value); // step is constant

        lastV = value;

        repartitionTemp.append((double)rep);
    }


//lastRepVal = rep;
    // Normalize repartition
    QList<double> repartition;
    for (auto&& v : repartitionTemp)
        repartition.append(v/rep);

    return repartition;
}
*/

QList<double> calculRepartition(const QMap<double, double>  &calib)
{
    QList<double> repartitionTemp;

    // we use long double type because
    // after several sums, the repartion can be in the double type range
    long double lastV = 0;
    long double rep = 0.;

#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    for (auto [key, value] : calib.asKeyValueRange()) {
        if (value != 0. && lastV != 0.)
            rep += (lastV + value); // step is constant

        lastV = value;

        repartitionTemp.append(rep);
    }
#else
    for (auto it = calib.begin(); it != calib.end(); ++it) {
        //double key = it.key();
        double value = it.value();
        if (value != 0. && lastV != 0.)
            rep += (lastV + value); // step is constant
        lastV = value;
        repartitionTemp.append(rep);
    }
#endif
    // Normaliser la répartition
    QList<double> repartition;
    repartition.reserve(repartitionTemp.size()); // Réserve de la mémoire

    std::transform(repartitionTemp.cbegin(), repartitionTemp.cend(), std::back_inserter(repartition),
                   [rep](double v) { return v / rep; });

    return repartition;
}

std::vector<double> calculRepartition(const std::map<double, double>  &calib)
{
    std::vector<double> repartitionTemp;

    // we use long double type because
    // after several sums, the repartion can be in the double type range
    long double lastV = 0;
    long double rep = 0.;

#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    for (auto [key, value] : calib) {
        if (value != 0. && lastV != 0.)
            rep += (lastV + value); // step is constant

        lastV = value;

        repartitionTemp.push_back(rep);
    }
#else
    for (auto it = calib.begin(); it != calib.end(); ++it) {
        //double key = it.key();
        double value = it.value();
        if (value != 0. && lastV != 0.)
            rep += (lastV + value); // step is constant
        lastV = value;
        repartitionTemp.append(rep);
    }
#endif

    // Normaliser la répartition
    std::vector<double> repartition;
    repartition.reserve(repartitionTemp.size()); // Réserve de la mémoire

    std::transform(repartitionTemp.cbegin(), repartitionTemp.cend(), std::back_inserter(repartition),
                   [rep](double v) { return v / rep; });


    return repartition;
}


Quartiles quartilesForRepartition(const QList<double> &repartition, const double tmin, const double step)
{
    Quartiles quartiles;
    if (repartition.size()<5) {
        quartiles.Q1 = tmin;
        quartiles.Q2 = tmin;
        quartiles.Q3 = tmin;
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

Quartiles quartilesForRepartition(const std::vector<double> &repartition, const double tmin, const double step)
{
    Quartiles quartiles;
    if (repartition.size()<5) {
        quartiles.Q1 = tmin;
        quartiles.Q2 = tmin;
        quartiles.Q3 = tmin;
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
std::pair<double, double> credibilityForTrace(const QList<double> &trace, double thresh, double& exactThresholdResult, const  QString description)
{
    (void) description;
    std::pair<double, double> credibility(0.,0.);
    exactThresholdResult = 0.;
    size_t n = trace.size();

    if (n == 1) {
        credibility.first = trace[0];
        credibility.second = trace[0];
        exactThresholdResult = 1.;
        return credibility;
    }

    if (thresh > 0 && n > 0) {
        double threshold = std::clamp(thresh, 0.0, 100.0);
        QList<double> sorted (trace);
        std::sort(sorted.begin(),sorted.end());

        const size_t numToRemove = (size_t)floor(n * (1. - threshold / 100.));
        exactThresholdResult = ((double)n - (double)numToRemove) / (double)n;

        double lmin = 0.;
        size_t foundJ = 0;

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
        exactThresholdResult = 1.;
        //return QPair<double, double>();
    }

        return credibility;
}


std::pair<double, double> credibilityForTrace(const std::vector<double> &trace, double thresh, double& exactThresholdResult, const QString description)
{
    (void) description;
    std::pair<double, double> credibility(0.,0.);
    exactThresholdResult = 0.;
    const size_t n = trace.size();
    if (n == 1) {
        credibility.first = trace[0];
        credibility.second = trace[0];
        return credibility;
    }

    if (thresh > 0 && n > 0) {
        double threshold = std::clamp(thresh, 0.0, 100.0);
        std::vector<double> sorted (trace);
        std::sort(sorted.begin(),sorted.end());

        size_t numToRemove = (size_t)floor(n * (1. - threshold / 100.));
        exactThresholdResult = ((double)n - (double)numToRemove) / (double)n;

        double lmin (0.);
        size_t foundJ (0);

        for (size_t j=0; j<=numToRemove; ++j) {
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

// Used in generateTempo for credibility
std::pair<double, double> credibilityForTrace(const QList<int> &trace, double thresh, double& exactThresholdResult, const QString description)
{
    (void) description;
    std::pair<double, double> credibility(0.,0.);
    exactThresholdResult = 0.;
    const size_t n = trace.size();
    if (n == 1) {
        credibility.first = trace[0];
        credibility.second = trace[0];
        return credibility;
    }

    if (thresh > 0 && n > 0) {
        double threshold = std::clamp(thresh, 0.0, 100.0);
        QList<int> sorted (trace);
        std::sort(sorted.begin(),sorted.end());

        size_t numToRemove = (size_t)floor(n * (1. - threshold / 100.));
        exactThresholdResult = static_cast<double>(n - numToRemove) / static_cast<double>(n);

        double lmin = 0.0;
        size_t foundJ (0);

        for (size_t j = 0; j<=numToRemove; ++j) {
            const double l = sorted[(n - 1) - numToRemove + j] - sorted[j];
            if ((lmin == 0.) || (l < lmin)) {
                foundJ = j;
                lmin = l;
            }
        }
        credibility.first = static_cast<double>(sorted[foundJ]);
        credibility.second = static_cast<double>(sorted[(n - 1) - numToRemove + foundJ]);
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
std::pair<double, double> timeRangeFromTraces(const QList<double> &trace1, const QList<double> &trace2, const double thresh, const QString description)
{
    (void) description;
    std::pair<double, double> range(- INFINITY, +INFINITY);
#ifdef DEBUG
    QElapsedTimer startTime;
    startTime.start();
#endif
    // limit of precision, to accelerate the calculus
    const double epsilonStep = 0.1/100.;

    // if thresh is equal 0 then return an QPair=(-INFINITY,+INFINITY)

    const size_t n = trace1.size();

    if ( (thresh > 0) && (n > 0) && ((unsigned)trace2.size() == n) ) {

        const double gamma = 1. - thresh/100.;

        double dMin = INFINITY;

        std::vector<double> traceAlpha (std::vector<double>(trace1.cbegin(), trace1.cend()) );
        std::vector<double> traceBeta (trace2.size());

        // 1 - map with relation Beta to Alpha
        std::multimap<double, double> betaAlpha;
        for(int i=0; i<trace1.size(); ++i)
            betaAlpha.insert(std::pair<double, double>(trace2.at(i), trace1.at(i)) );

        std::copy(trace2.begin(), trace2.end(), traceBeta.begin());

        // keep the beta trace in the same position of the Alpha, so we need to sort them with there values of alpha
        std::sort(traceBeta.begin(), traceBeta.end(), [&betaAlpha](const double i, const double j){ return betaAlpha.find(i)->second < betaAlpha.find(j)->second  ;} );

        std::sort(traceAlpha.begin(), traceAlpha.end());

        std::vector<double> betaUpper(n);

        // 2- loop on Epsilon to look for a and b with the smallest length
        for (double epsilon = 0.; epsilon <= gamma; ) {

            // original calcul according to the article const float ha( (traceAlpha.size()-1)*epsilon +1 );
            // We must decrease of 1 because the array begin at 0
            const double ha( (traceAlpha.size()-1)*epsilon);

            const size_t haInf ( floor(ha) );
            const size_t haSup ( ceil(ha) );

            const double a = traceAlpha.at(haInf) + ( (ha-haInf)*(traceAlpha.at(haSup)-traceAlpha.at(haInf)) );

            // 3 - copy only value of beta with alpha greater than a(epsilon)
            const size_t alphaIdx = ha==haInf ? haInf:haSup;

            const size_t remainingElemt =  n - alphaIdx;
            betaUpper.resize(remainingElemt);   // allocate space

            // traceBeta is sorted with the value alpha join
            auto it = std::copy( traceBeta.begin()+ alphaIdx, traceBeta.end(), betaUpper.begin() );

            const size_t betaUpperSize = (size_t) std::distance(betaUpper.begin(), it);

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

std::pair<double, double> timeRangeFromTraces(const std::vector<double> &trace1, const std::vector<double> &trace2, const double thresh, const QString description)
{
    (void) description;
    std::pair<double, double> range(- INFINITY, +INFINITY);
#ifdef DEBUG
    QElapsedTimer startTime;
    startTime.start();
#endif
    // limit of precision, to accelerate the calculus
    const double epsilonStep = 0.1/100.;

    // if thresh is equal 0 then return an QPair=(-INFINITY,+INFINITY)

    const size_t n = trace1.size();

    if ( (thresh > 0) && (n > 0) && ((unsigned)trace2.size() == n) ) {

        const double gamma = 1. - thresh/100.;

        double dMin = INFINITY;

        std::vector<double> traceAlpha (trace1);
        std::vector<double> traceBeta (trace2.size());

        // 1 - map with relation Beta to Alpha
        std::multimap<double, double> betaAlpha;
        for(size_t i = 0; i<trace1.size(); ++i)
            betaAlpha.insert(std::pair<double, double>(trace2.at(i), trace1.at(i)) );

        std::copy(trace2.begin(), trace2.end(), traceBeta.begin());

        // keep the beta trace in the same position of the Alpha, so we need to sort them with there values of alpha
        std::sort(traceBeta.begin(), traceBeta.end(), [&betaAlpha](const double i, const double j){ return betaAlpha.find(i)->second < betaAlpha.find(j)->second  ;} );

        std::sort(traceAlpha.begin(), traceAlpha.end());

        std::vector<double> betaUpper(n);

        // 2- loop on Epsilon to look for a and b with the smallest length
        for (double epsilon = 0.; epsilon <= gamma; ) {

            // original calcul according to the article const float ha( (traceAlpha.size()-1)*epsilon +1 );
            // We must decrease of 1 because the array begin at 0
            const double ha( (traceAlpha.size()-1)*epsilon);

            const size_t haInf ( floor(ha) );
            const size_t haSup ( ceil(ha) );

            const double a = traceAlpha.at(haInf) + ( (ha-haInf)*(traceAlpha.at(haSup)-traceAlpha.at(haInf)) );

            // 3 - copy only value of beta with alpha greater than a(epsilon)
            const size_t alphaIdx = ha==haInf ? haInf:haSup;

            const size_t remainingElemt =  n - alphaIdx;
            betaUpper.resize(remainingElemt);   // allocate space

            // traceBeta is sorted with the value alpha join
            auto it = std::copy( traceBeta.begin()+ alphaIdx, traceBeta.end(), betaUpper.begin() );

            const size_t betaUpperSize = (size_t) std::distance(betaUpper.begin(), it);

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

std::pair<double, double> transitionRangeFromTraces(const QList<double> &trace1, const QList<double> &trace2, const double thresh, const QString description)
{
    return timeRangeFromTraces(trace1, trace2, thresh, description);
}

std::pair<double, double> transitionRangeFromTraces(const std::vector<double> &trace1, const std::vector<double> &trace2, const double thresh, const QString description)
{
    return timeRangeFromTraces(trace1, trace2, thresh, description);
}

/**
 * @brief gapRangeFromTraces find the gap between two traces, if there is no solution corresponding to the threshold, we return a QPair=(-INFINITY,+INFINITY)
 * @param traceBeta QList of double corresponding to the first trace
 * @param traceAlpha QList of double corresponding to the second trace
 * @param thresh Threshold to obtain
 * @param description a simple text
 * @return
 */
std::pair<double, double> gapRangeFromTraces(const QList<double> &traceEnd, const QList<double> &traceBegin, const double thresh, const QString description)
{
    (void) description;
#ifdef DEBUG
    QElapsedTimer startTime ;
    startTime.start();
#endif

    std::pair<double, double> range = std::pair<double, double>(- INFINITY, + INFINITY);

    // limit of precision, to accelerate the calculus, we set the same as RChronoModel
    const double epsilonStep = 0.1/100.;

    const auto  n = traceBegin.size();

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
        for (auto i=0; i<traceBegin.size(); ++i)
            alphaBeta.insert(std::pair<double, double>(traceAlpha.at(i), traceBeta.at(i)) );

        // keep the beta trace in the same position of the Alpha, so we need to sort them with there values of alpha
        std::sort(traceAlpha.begin(), traceAlpha.end(), [&alphaBeta](const double i, const double j){ return alphaBeta.find(i)->second < alphaBeta.find(j)->second  ;} );

        std::sort(traceBeta.begin(), traceBeta.end());

        std::vector<double> alphaUnder(n);

        // 2- loop on Epsilon to look for a and b with the smallest length
        // with a const epsilonStep increment, we not reach exactly gamma.
        // So we have to go hover gamma to find the value for exactly gamma

        for (double epsilon = 0.; epsilon <= gamma; ) {

            const double aEpsilon = 1. - epsilon;

            // Linear interpolation according to R quantile( type=7)
            // We must decrease of 1 from the original formula because the array begin at 0
            const double ha( ((double)traceBeta.size()-1.) * aEpsilon);

            const long long haInf( floor(ha) );
            const long long haSup( ceil(ha) );

            if ((haSup > (int)traceBeta.size()) || (haInf > (int)traceBeta.size()))
                return range;

            if ((haInf < 0) || (haSup < 0))
                return range;

            const double a = traceBeta.at(haInf) + ( (ha-(double)haInf)*(traceBeta.at(haSup)-traceBeta.at(haInf)) );

            // 3 - Copy only value of beta with alpha smaller than a(epsilon)!
            const long long alphaIdx(haSup < n ? haSup : n-1 );//( ha == haInf ? haInf : haSup );//( ha == haSup ? haSup : haInf );// //

            const long long remainingElemt ( alphaIdx );
            alphaUnder.resize(remainingElemt);   // allocate space

            // traceAlpha is sorted with the value alpha join
            auto it = std::copy( traceAlpha.begin(), traceAlpha.begin() + alphaIdx, alphaUnder.begin() );

            const long long alphaUnderSize = (long long) std::distance(alphaUnder.begin(), it);

            alphaUnder.resize(alphaUnderSize);  // shrink container to new size

            // 4- We sort all the array
            std::sort(alphaUnder.begin(), alphaUnder.end());

           // 5 - Calcul b
            const double bEpsilon( (gamma-epsilon)/(1.-epsilon) );

            // Linear interpolation like in R quantile( type=7)

            const size_t hb( ((double)alphaUnder.size()-1.)* bEpsilon );
            const size_t hbInf( floor(hb) );
            const size_t hbSup( ceil(hb) );

            if ((hbSup > alphaUnder.size()) || (hbInf > alphaUnder.size()))
                return range;
            
            //if ((hbInf < 0) || (hbSup <0)) // impossible
            //    return range;

            const double b = alphaUnder.at(hbInf) + ( ((double)hb -(double)hbInf)*(alphaUnder.at(hbSup)-alphaUnder.at(hbInf)) );

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
     qDebug()<<"gapRangeFromTraces done in " + DHMS(startTime.elapsed());

#endif

    return range;
}

std::pair<double, double> gapRangeFromTraces(const std::vector<double> &traceEnd, const std::vector<double> &traceBegin, const double thresh, const QString description)
{
    (void) description;
#ifdef DEBUG
    QElapsedTimer startTime ;
    startTime.start();
#endif

    std::pair<double, double> range = { -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity() };

    // limit of precision, to accelerate the calculus, we set the same as RChronoModel
    const double epsilonStep = 0.1/100.;

    const auto  n = traceBegin.size();

    if ( (thresh > 0.f) && (n > 0) && (traceEnd.size() == n) ) {

        const double gamma = 1. - thresh/100.;

        double dMax(0.);

        // We must change the type (float to double) to increase the precision
        std::vector<double> traceAlpha = traceBegin; // Copy only once
        std::vector<double> traceBeta = traceEnd; // Copy only once

        // 1 - map with relation Alpha to Beta
        std::multimap<double, double> alphaBeta;
        for (size_t i = 0; i < n; ++i) {
            alphaBeta.emplace(traceAlpha[i], traceBeta[i]);
        }

       // for (size_t i=0; i<traceBegin.size(); ++i)
         //   alphaBeta.insert(std::pair<double, double>(traceAlpha.at(i),traceBeta.at(i)) );

        // keep the beta trace in the same position of the Alpha, so we need to sort them with there values of alpha
        std::sort(traceAlpha.begin(), traceAlpha.end(), [&alphaBeta](const double i, const double j){ return alphaBeta.find(i)->second < alphaBeta.find(j)->second  ;} );

        std::sort(traceBeta.begin(),traceBeta.end());

        std::vector<double> alphaUnder(n);

        // 2- loop on Epsilon to look for a and b with the smallest length
        // with a const epsilonStep increment, we not reach exactly gamma.
        // So we have to go hover gamma to find the value for exactly gamma

        for (double epsilon = 0.; epsilon <= gamma; ) {

            const double aEpsilon = 1. - epsilon;

            // Linear interpolation according to R quantile( type=7)
            // We must decrease of 1 from the original formula because the array begin at 0
            const double ha( ((double)traceBeta.size()-1.) * aEpsilon);


            const long long haInf = std::floor(ha);
            const long long haSup = std::ceil(ha);

            if (haInf < 0 || haSup >= static_cast<long long>(traceBeta.size())) {
                return range;
            }

            const double a = traceBeta[haInf] + (ha - haInf) * (traceBeta[haSup] - traceBeta[haInf]);


            if ((haSup > static_cast<long long>(traceBeta.size())) || (haInf > static_cast<long long>(traceBeta.size())))
                return range;

            if ((haInf < 0) || (haSup < 0))
                return range;


            // 3 - Copy only value of beta with alpha smaller than a(epsilon)!
            const long long alphaIdx(haSup < (double)n ? haSup : (double)(n-1) );//( ha == haInf ? haInf : haSup );//( ha == haSup ? haSup : haInf );// //

            const long long remainingElemt ( alphaIdx );
            alphaUnder.resize(remainingElemt);   // allocate space

            // traceAlpha is sorted with the value alpha join
            auto it = std::copy( traceAlpha.begin(), traceAlpha.begin() + alphaIdx, alphaUnder.begin() );

            const long long alphaUnderSize = std::distance(alphaUnder.begin(), it);

            alphaUnder.resize(alphaUnderSize);  // shrink container to new size

            // 4- We sort all the array
            std::sort(alphaUnder.begin(), alphaUnder.end());

            // 5 - Calcul b
            const double bEpsilon( (gamma-epsilon)/(1.-epsilon) );

            // Linear interpolation like in R quantile( type=7)

            const size_t hb( (static_cast<double>(alphaUnder.size())-1.)* bEpsilon );
            const size_t hbInf( floor(hb) );
            const size_t hbSup( ceil(hb) );

            if ((hbSup > alphaUnder.size()) || (hbInf > alphaUnder.size()))
                return range;

            //if ((hbInf < 0) || (hbSup <0)) // impossible
            //    return range;

            const double b = alphaUnder[hbInf] + ( (static_cast<double>(hb) -static_cast<double>(hbInf))*(alphaUnder[hbSup]-alphaUnder[hbInf]) );

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
    qDebug()<<"gapRangeFromTraces done in " + DHMS(startTime.elapsed());

#endif

    return range;
}


const QString interval_to_text(const QPair<double, QPair<double, double> > &interval,  DateConversion conversionFunc, const bool forCSV)
{
    double inter1 = (conversionFunc ? conversionFunc(interval.second.first) : interval.second.first );
    double inter2 = (conversionFunc ? conversionFunc(interval.second.second) : interval.second.second );
    if (inter1>inter2)
        std::swap(inter1, inter2);

    if (forCSV)
         return stringForCSV(inter1) + AppSettings::mCSVCellSeparator + stringForCSV(inter2) + AppSettings::mCSVCellSeparator + stringForCSV(interval.first*100.);
    else
         return "[ " + stringForLocal(inter1) + " ; " + stringForLocal(inter2) + " ] (" + stringForLocal(interval.first*100.) + "%)";

}

const QString get_HPD_text_from_mapping(const std::map<double, double> &mapping, const QString &unit, DateConversion conversionFunc, const bool forCSV)
{
    if (mapping.size() == 0)
        return "";

    double th = 0;
    QList<QPair<double, QPair<double, double> > > intervals = intervals_hpd_from_mapping(mapping, th);
    QStringList results;

    double min_inter = (conversionFunc ? conversionFunc(intervals.at(0).second.first) : intervals.at(0).second.first );
    double max_inter = (conversionFunc ? conversionFunc(intervals.at(0).second.second) : intervals.at(0).second.second );

    if (min_inter <= max_inter) {
        for (auto&& interval : intervals) {
            results << interval_to_text(interval, conversionFunc, forCSV);
        }

    } else {
        for (auto&& interval = intervals.crbegin(); interval != intervals.crend(); interval++) {
            results << interval_to_text(*interval, conversionFunc, forCSV);
        }
    }
    //----
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

const QString get_HPD_text(const QList<QPair<double, QPair<double, double> >> &intervals, const QString &unit,  DateConversion conversionFunc, const bool forCSV)
{
    if (intervals.size() == 0)
        return "";

    QStringList results;

    double min_inter = (conversionFunc ? conversionFunc(intervals.at(0).second.first) : intervals.at(0).second.first );
    double max_inter = (conversionFunc ? conversionFunc(intervals.at(0).second.second) : intervals.at(0).second.second );

    if (min_inter <= max_inter) {
        for (auto&& interval : intervals) {
            results << interval_to_text(interval, conversionFunc, forCSV);
        }

    } else {
        for (auto&& interval = intervals.crbegin(); interval != intervals.crend(); interval++) {
            results << interval_to_text(*interval, conversionFunc, forCSV);
        }
    }
    //----
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
 * @brief intervals_hpd_from_mapping
 * @param area_mapping must be normalize 100% = 1
 * @param real_thresh return the remaining real surface between [0, 1]
 * @return a list of interval
 */
QList<QPair<double, QPair<double, double> > > intervals_hpd_from_mapping(const std::map<double, double> &area_mapping, double &real_thresh)
{
    QList<QPair<double, QPair<double, double> >> intervals;

    if (area_mapping.size() == 0) {
        real_thresh = 0.;
        return intervals;
    }

    if (area_mapping.size() == 1) {
        QPair<double, QPair<double, double> > inter;
        inter.first = 100.;
        inter.second = QPair<double, double> (area_mapping.begin()->first, area_mapping.begin()->second);
        intervals.append(inter);
        real_thresh = 1.;
        return intervals;
    }

    std::map<double, double>::const_iterator it (area_mapping.cbegin());
    bool inInterval = false;
    QPair<double, double> curInterval;

    real_thresh = std::accumulate(area_mapping.begin(), area_mapping.end(), 0., [](double sum, std::pair<double, double> p) {return sum + p.second;});
    double areaCur = 0.;

    while (it != area_mapping.end()) {

        if (it->second != 0. && !inInterval) {
            inInterval = true;
            curInterval.first = it->first;

            areaCur = it->second;// egal area

        } else if (inInterval) {


            if (it->second == 0. ) {
                inInterval = false;
                curInterval.second = std::prev(it)->first;

                QPair<double, QPair<double, double> > inter;
                inter.first = areaCur;
                inter.second = curInterval;
                intervals.append(inter);
                areaCur = 0.;
                //std::cout<<"Function::intervalsForHpd"<<inter.first<< inter.second.first<<inter.second.second;

            } else {
                curInterval.second = it->first;
                areaCur += it->second;// egal area

            }
        }
        it++;
    }

    if (inInterval) { // Correction to close unclosed interval
        curInterval.second = std::prev(it)->first;
        QPair<double, QPair<double, double> > inter;
        inter.first = real_thresh;
        inter.second = curInterval;
        intervals.append(inter);
    }

    return intervals;
}

QList<QPair<double, QPair<double, double> > > intervals_hpd_from_mapping(const std::map<double, double> &area_mapping)
{
    double thresh;
    return intervals_hpd_from_mapping(area_mapping, thresh);
}

#pragma mark Calcul Matriciel

// useless function
std::vector<t_matrix> initVector(size_t n)
{
    return std::vector<t_matrix>(n, 0.);
}

std::vector<long double> initLongVector(size_t n)
{
     return std::vector<long double>(n, 0.);
}

/*
std::vector<std::vector<int>> initIntMatrix(size_t rows, size_t cols)
{
    return std::vector<std::vector<int>> (rows, std::vector<int>(cols, 0.));
}

std::vector<std::vector<double>> initMatrix(size_t rows, size_t cols)
{
    return std::vector<std::vector<double>> (rows, std::vector<double>(cols, 0.));
}
*/

void resizeMatrix(std::vector<std::vector<double>> &matrix,  size_t rows, size_t cols)
{
    matrix.resize( rows );
    for ( auto&& it = matrix.begin(); it != matrix.end(); ++it) {
        it->resize( cols );
    }
}

void resizeLongMatrix(Matrix2D& matrix, size_t rows, size_t cols)
{
    // Créer une nouvelle matrice avec les dimensions spécifiées
    Matrix2D resMatrix(rows, cols);

    // Déterminer le nombre de lignes à copier
    size_t minRows = std::min(size_t(matrix.rows()), rows);

    // Copier les données de la matrice d'origine dans la nouvelle matrice
    for (size_t i = 0; i < minRows; ++i) {
        // Copier les colonnes, en s'assurant de ne pas dépasser le nombre de colonnes
        for (size_t j = 0; j < std::min(size_t(matrix.cols()), cols); ++j) {
            resMatrix(i, j) = matrix(i, j);
        }
    }

    // Remplacer la matrice d'origine par la nouvelle matrice redimensionnée
    matrix = resMatrix;
}


Matrix2D seedMatrix(const Matrix2D& matrix, size_t shift)
{
    if (shift == 0)
        return matrix;

    size_t n = matrix.rows() - 2 * shift;
    size_t m = matrix.cols() - 2 * shift;
#ifdef DEBUG
    // Vérification pour éviter des dimensions négatives
    if (n <= 0 || m <= 0) {
        throw std::invalid_argument("[seedMatrix] Shift is too large for the given matrix dimensions.");
    }
#endif
    Matrix2D resMatrix(n, m);

    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < m; ++j)
            resMatrix(i, j) = matrix(i + shift, j + shift);
    }

    return resMatrix;
}


Matrix2D remove_bands_Matrix(const Matrix2D& matrix, size_t shift)
{
    if (shift == 0)
        return matrix;

    size_t nb_row = matrix.rows();
    size_t nb_col = matrix.cols() - 2 * shift;
    Matrix2D resMatrix(nb_row, nb_col);

    /*for (size_t i = 0; i < nb_row; i++) {
        resMatrix[i] = matrix[i][std::slice(shift, nb_col, 1)];
    }*/
    for (size_t i = 0; i < nb_row; ++i) {
        for (size_t j = 0; j < nb_col; ++j)
            resMatrix(i, j) = matrix(i , j + shift);
    }
    return resMatrix;
}




/**
 * @brief Calcule la trace d'une matrice carrée en utilisant l'algorithme de Kahan
 *
 * La trace d'une matrice carrée \f$ A \f$ est définie par :
 * \f[
 * \mathrm{Tr}(A) = \sum_{i=1}^{n} A_{ii}
 * \f]
 *
 * Cette fonction utilise l'algorithme de Kahan pour améliorer la précision
 * de la sommation en virgule flottante.
 *
 * @param mat Matrice 2D (valarray de valarray)
 * @return La trace de la matrice
 * @throws std::invalid_argument si la matrice n'est pas carrée
 */
t_matrix trace_kahan(const Matrix2D& mat)
{
    size_t n = mat.rows();
#ifdef DEBUG
    if (n == 0 || mat.cols() != mat.rows())
        throw std::invalid_argument("[trace_kahan] La matrice doit être carrée et non vide.");
#endif
    t_matrix sum = 0.0;
    t_matrix c = 0.0; // Compensation pour l'erreur perdue

    for (size_t i = 0; i < n; ++i) {
        t_matrix y = mat(i, i) - c;
        t_matrix t = sum + y;
        c = (t - sum) - y;
        sum = t;
    }

    return sum;
}

t_matrix determinant(const Matrix2D& matrix, size_t shift)
{
    size_t n = matrix.rows();
    t_matrix det;

    if (n - 2*shift == 1) {
          det = matrix(shift, shift);

    } else if (n - 2*shift == 2) {
        det = matrix(shift, shift) * matrix(1+shift, 1+shift) - matrix(1+shift, shift) * matrix(shift, 1+shift);

    } else {
        Matrix2D matrix2;
        if (shift == 0)
            matrix2 = matrix;
        else
            matrix2 = seedMatrix(matrix, shift);

        n = matrix2.rows();

        Matrix2D matTmp (n-1, n-1);

        det = 0.;
        int j2;
        for (size_t j1 = 0; j1< n; j1++) {
            for (size_t i = 1; i < n; i++) {
                j2 = 0;
                for (size_t j = 0; j < n; j++) {
                    if (j == j1)
                        continue;
                    matTmp(i-1, j2) = matrix2(i, j);
                    j2++;
                }
            }
            det += pow(-1.0, j1+2.0) * matrix2(0, j1) * determinant(matTmp, 0);

        }

    }
#ifdef DEBUG
    if (det == 0) {
           throw std::runtime_error("[Function::determinant] det ==0");
       }
#endif

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
t_matrix determinant_gauss(const Matrix2D& matrix, size_t shift)
{
    size_t n = matrix.rows();
    t_matrix det;

    if (n - 2*shift == 1) {
          det = matrix(shift, shift);

    } else if (n - 2*shift == 2) {
        det = matrix(shift, shift) * matrix(1+shift, 1+shift) - matrix(1+shift, shift) * matrix(shift, 1+shift);

    } else {
        Matrix2D matrix2 = seedMatrix(matrix, shift);
        n = matrix2.rows();

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
      t_matrix  coeffMax;

      // ( etape 1 )
      rankMax = j;
      for (rank=j+1;rank<n;rank++) {
        if (fabs(matTmp(rankMax, j)) < fabs(matTmp(rank, j))) {
          rankMax = rank;
        }
      }

      coeffMax = matTmp(rankMax, j);
      if (fabs(coeffMax) <= std::numeric_limits<long double>::epsilon())  {
         return( 0.);
      }
      // ( etape 2 )
      if (rankMax != j)  {
        t_matrix tmp;
        for (i=j; i<n; i++) {
          tmp = matTmp(j, i);
          matTmp(j, i) = matTmp(rankMax, i);
          matTmp(rankMax, i) = tmp;
          }
        det *= -1.;
      }

      det *= coeffMax;
      // ( etape 3 )
      t_matrix coeff;
      for (rank=j+1; rank<n; rank++) {
        coeff = matTmp(rank, j)/coeffMax;
        for ( i=j; i<n; i++)  {
          matTmp(rank, i) -= coeff*matTmp(j, i);
        }
      }

    }

    det *= matTmp(n-1, n-1);
   }


#ifdef DEBUG
    if (det == 0) {
              throw std::runtime_error("[Function] determinant() det == 0");
          }
#endif
      //-----
    return (det);
}
// On suppose une matrice quelconque M*N
 Matrix2D transpose0(const Matrix2D &A)
{
   /*size_t n = A.size();
   size_t m = A[0].size();
   Matrix2D TA  = initMatrix2D(m, n);

#ifdef _OPENMP
#pragma omp parallel for
    for (size_t j = 0; j < m; ++j) {
        for (size_t i = 0; i < n; ++i) {
            TA[j][i] = A[i][j];
        }
    }
#else
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < m; ++j) {
            TA[j][i] = A[i][j];
        }
    }
#endif
*/

   return A.transpose();

}


// On suppose une matrice carrée N*N
Matrix2D transpose(const Matrix2D& matrix, const size_t nbBandes)
{
    const size_t dim = matrix.rows();
    Matrix2D result (dim, dim);

    const int k = floor((nbBandes-1)/2); // calcul du nombre de bandes

    for (size_t i = 0; i < dim; ++i) {
        int j1 = std::max(0, static_cast<int>(i - k));
        int j2 = std::min(dim-1, i + k);

        for (int j = j1; j <= j2; ++j) {
            result(j, i) = matrix(i, j);
        }

    }
    return result;
}

// Multiplication d'une matrice pleine A par une matrice diagonale D (A×D)
Matrix2D multiMatParDiag0(const Matrix2D& matrix, const DiagonalMatrixLD& diag)
{
    const size_t n = matrix.rows();
    const size_t m = matrix.cols();

    Matrix2D result = Matrix2D::Zero(n, m);

    const auto& diagVec = diag.diagonal();

    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < m; ++j) {
            result(i, j) = matrix(i, j) * diagVec[j];
        }
    }

    return result;
}


Matrix2D multiMatParDiag(const Matrix2D& matrix, const DiagonalMatrixLD& diag, size_t nbBandes)
{
    const size_t dim = matrix.rows();
    Matrix2D result = Matrix2D::Zero (dim, dim);
    const size_t k = (nbBandes - 1) / 2; // calcul du nombre de bandes

    for (size_t i = 0; i < dim; ++i) {
        int j1 = std::max(0, static_cast<int>(i) - static_cast<int>(k));
        int j2 = std::min(static_cast<int>(dim) - 1, static_cast<int>(i) + static_cast<int>(k));

        for (int j = j1; j <= j2; ++j) {
            result(i, j) = matrix(i, j) * diag.diagonal()[j];
        }
    }
    return result;
}


// Multiplication d'une matrice diagonale par une matrice pleine
// La matrice diagonale doit avoir autant de ligne que la matrice pleine
Matrix2D multiDiagParMat0(const DiagonalMatrixLD &diag, const Matrix2D &matrix)
{
    const long rows = matrix.rows();
    const long cols = matrix.cols();
#ifdef DEBUG
    if (rows != diag.rows()) {
        std::cout << "[Function::multiDiagParMat0] matricx.row != diag.size" << std::endl;
        throw std::runtime_error("[Function::multiDiagParMat0] matricx.row != diag.size");
    }
#endif

    Matrix2D result = Matrix2D::Zero (rows, cols);

    //  diag est un vecteur avec un nombre d'éléments égal au nombre de lignes
    for (long i = 0; i < rows; ++i) {
        auto diag_i = diag.diagonal()[i];
        for (long j = 0; j < cols; ++j) {
            result(i, j) = diag_i * matrix(i, j); // Appliquer l'élément diagonal à la ligne
        }
    }
    return result;
}

// Multiplication d'une matrice diagonale par une matrice de bande
Matrix2D multiDiagParMat(const DiagonalMatrixLD& diag, const Matrix2D& matrix, const size_t nbBandes)
{
    const long dim = matrix.rows();
    Matrix2D result = Matrix2D::Zero (dim, dim);
    const long k = (nbBandes - 1) / 2; // calcul du nombre de bandes

    for (long i = 0; i < dim; ++i) {
        long j1 = std::max(long(0), i - k);
        long j2 = std::min(dim - 1, i + k);

        auto diag_i = diag.diagonal()[i];

        for (long j = j1; j <= j2; ++j) {
            result(i, j) = diag_i * matrix(i, j);
        }
    }
    return result;
}



/**
 * @brief multiMatParVec - calcul différent du produit matrice par Diagonal
 * @param matrix
 * @param vec
 * @param nbBandes = k1+k2+1
 * @return
 */
std::vector<t_matrix> multiMatParVec(const Matrix2D &matrix, const std::vector<t_matrix> &vec, const size_t nbBandes)
{
    const int dim = (int)vec.size();
    std::vector<t_matrix> result;
    const int  k = floor((nbBandes-1)/2); // calcul du nombre de bandes
    double sum;
    for (int i = 0; i < dim; ++i) {
        sum = 0.;
        int  j1 = std::max(0, i - k);
        int  j2 = std::min(dim-1, i + k);

        for (int j = j1; j <= j2; ++j) {
            sum += matrix(i, j) * vec[j];
        }
        result.push_back(sum);
    }
    return result;
}

/* std::vector<t_matrix> multiMatParVec(const Matrix2D& matrix, const MatrixDiag &vec, const size_t nbBandes)
{
    const int dim = static_cast<int>(vec.size());
    std::vector<t_matrix> result;
    const int  k = floor((nbBandes-1)/2); // calcul du nombre de bandes
    t_matrix sum;
    for (int i = 0; i < dim; ++i) {
        sum = 0.0;
        int  j1 = std::max(0, i - k);
        int  j2 = std::min(dim-1, i + k);
        const t_matrix* matrix_i = begin(matrix[i]);
        for (int j = j1; j <= j2; ++j) {
            sum += matrix_i[j] * vec[j];
        }
        result.push_back(sum);
    }
    return result;
}
*/


Matrix2D addMatEtMat0(const Matrix2D &matrix1, const Matrix2D &matrix2)
{
    const long dim = matrix1.cols();

    Matrix2D result = matrix1;

    for (long i = 0; i< result.rows(); i++) {
         for (long j = 0; j < dim; ++j) {
            result(i, j) +=  matrix2(i, j);
        }

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
Matrix2D addMatEtMat(const Matrix2D &matrix1, const Matrix2D &matrix2, const size_t nbBandes2)
{

    const long rows = matrix1.rows();
    const long cols = matrix1.cols();

    // Vérification dimensions compatibles
    assert(matrix2.rows() == rows && matrix2.cols() == cols && "Matrices doivent avoir les mêmes dimensions");

    const long k = nbBandes2 / 2; // Supposé que bandwidth = 2k + 1 pour une bande symétrique

    Matrix2D result = matrix1;

    for (long i = 0; i < rows; ++i) {
        const long j_min = (i >= k) ? i - k : 0;
        const long j_max = std::min(i + k, cols - 1);

        for (long j = j_min; j <= j_max; ++j) {
            result(i, j) += matrix2(i, j);
        }
    }

    return result;



}

Matrix2D addIdentityToMat(const Matrix2D& matrix)
{
    const size_t dim = matrix.rows();
    Matrix2D result = matrix;

    for (size_t i = 0; i < dim; ++i)
        result(i, i) += 1.0L;

    return result;
}


// Fonction pour calculer la forme quadratique d'une matrice
t_matrix quadratic_form(const Matrix2D& A, const std::vector<t_matrix>& X)
{
    size_t n = A.rows();
    t_matrix result = 0.0;
    for (size_t i = 0; i < n; i++) {
        t_matrix sum = 0.0;
        for (size_t j = 0; j < n; j++) {
            sum += A(i, j) * X[j];
        }
        result += sum * X[i];
    }
    return result;
}

t_matrix quadratic_form(const Matrix2D& K, const Matrix2D& Y)
{
    /* Algo Schoolbook
    long n_points = Y.rows();
    long n_components = Y.cols();

    Matrix2D KY = K * Y;

    t_matrix som = 0.0;
    for (long i = 0; i < n_points; ++i) {
        for (long j = 0; j < n_components; ++j) {
            som += KY(i, j) * Y(i, j);
        }
    }
    return  som;
    */
    //,Optimization with Eigen

    return (K * Y).cwiseProduct(Y).sum();


}


Matrix2D multiConstParMat(const Matrix2D& matrix, const double c, const size_t nbBandes)
{
    /*const size_t i_max = static_cast<size_t>(matrix.rows())-1;
    Matrix2D result = matrix;
    const size_t k = floor((nbBandes-1)/2.0); // calcul du nombre de bandes

    t_matrix cL = static_cast<t_matrix>(c);
    for (size_t i = 0; i< result.size(); i++) {
        size_t j1 = std::max(size_t(0), i - k);
        size_t j2 = std::min(i_max, i + k);
        for (size_t j = j1; j <= j2; ++j) {
            result(i, j) *= cL ;
        }
        i++;
    }
    return result;*/

    return c * matrix;
}


Matrix2D multiMatParMat0(const Matrix2D& matrix1, const Matrix2D& matrix2)
{
   /* const size_t nl1 = matrix1.rows();
    //const int nc1 = matrix1[0].size();
    const size_t nl2 = (int)matrix2.rows();
    const size_t nc2 = (int)matrix2.cols();
    // nc1 doit etre égal à nl2
    Matrix2D result (nl1, nc2);

    t_matrix sum;

    for (size_t i = 0; i < nl1; ++i) {


        for (size_t j = 0; j < nc2; ++j) {
            sum = 0;
            for (size_t k = 0; k < nl2; ++k) {

                sum += matrix1(i, k) * matrix2(k, j);

            }
            result(i, j) = sum;
        }
    }
    //return result;
    */

    return matrix1 * matrix2;
}

Matrix2D multiConstParMat0(const Matrix2D &matrix, const double c)
{
    /*const size_t n = matrix.rows() ;
    Matrix2D result (n, n);
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {
            result(i, j) = c * matrix(i, j);
        }
    }*/

    return c* matrix; //result;
}


Matrix2D addDiagToMat(const DiagonalMatrixLD &diag, Matrix2D matrix)
{
    const size_t dim = matrix.rows();
    Matrix2D result = matrix;
    for (size_t i = 0; i < dim; ++i)
        result(i, i) += diag.diagonal()[i];

    return result;
}


Matrix2D soustractMatToIdentity(const Matrix2D& matrix)
{
    const size_t n = matrix.rows();
    Matrix2D result = matrix;
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < size_t(result.cols()); j++)
            result(i, j) *= -1.0;
    }
    for (size_t i = 0; i < n; ++i)
        result(i, i) += 1.;

    return result;
}


/*
 * Naive implementation with std::inner_product optimization
 * using the schoolbook algorithm
 */
Matrix2D multiplyMatrix_Naive(const Matrix2D& a, const Matrix2D& b)
{
    /*const size_t a_row = a.rows();
    const size_t a_col = a.cols();
    const size_t b_row = b.rows();
    const size_t b_col = b.cols();
    if (a_col != b_row)
        throw std::runtime_error("[Function::multiplyMatrix_Naive] a.col != b.row");

    Matrix2D c(a_row, b_col);

    for (size_t i = 0; i < a_row; i++) {
        for (size_t j = 0; j < b_col; j++) {
            c(i, j) = std::inner_product(std::begin(a[i]), std::end(a[i]),
                                         std::begin(b[0]) + j, 0.0L,
                                         std::plus<t_matrix>(),
                                         std::multiplies<t_matrix>());
        }
    }*/
    return a * b;
}




/*
 * from RenCurve software
 * is Winograd algorithm
 */
Matrix2D multiMatParMat(const Matrix2D& matrix1, const Matrix2D& matrix2, const size_t nbBandes1, const size_t nbBandes2)
{
    const size_t dim = matrix1.rows();
    Matrix2D result (dim, dim);

    const size_t bande1 = floor((nbBandes1-1)/2);
    const size_t bande2 = floor((nbBandes2-1)/2);
    const size_t bandeRes = bande1 + bande2 +1;

    for (size_t i = 0; i < dim ; ++i) {
        size_t j1 = std::max(size_t(0), i -  bandeRes);
        size_t j2 = std::min(dim, i + bandeRes);

        size_t k1 = std::max(size_t(0), i - bandeRes);
        size_t k2 = std::min(dim, i + bandeRes);

        for (size_t j = j1; j < j2; ++j) {
            t_matrix sum = 0.L;
            for (size_t k = k1; k < k2; ++k) {
                sum += matrix1(i, k) * matrix2(k, j);
            }
            result(i, j) = sum;
        }
    }
    return result;
}




/* Winograd algorithm with bandwidth management
 * https://en.wikipedia.org/wiki/Computational_complexity_of_matrix_multiplication
 * https://en.wikipedia.org/wiki/Matrix_multiplication_algorithm
 * A band matrix with Bandwidth=0 is a diagonal matrix
 * faster than schoolbook algorithm
 */
Matrix2D multiplyMatrixBanded_Winograd(const Matrix2D& a, const Matrix2D& b, const int bandwidth)
{
#ifdef DEBUG
    const size_t a_row = a.rows();
    const size_t a_col = a.cols();
    const size_t b_row = b.rows();
    const size_t b_col = b.cols();
    if (a_row != a_col || b_row != b_col || a_row != b_row) {
        std::cerr << "[Function multiplyMatrixBanded_Winograd] matrices are not square" << std::endl;
    }
#endif

    const size_t n = a.rows();
    Matrix2D c = Matrix2D::Zero(n, n);

    for (size_t i = 0; i < n-bandwidth; ++i) {
        size_t j1 = std::max(size_t(0), i - bandwidth -1);
        size_t j2 = std::min(n, i + bandwidth +1);
        size_t k1 = std::max(size_t(0), i - bandwidth -1);
        size_t k2 = std::min(n, i + bandwidth +1);

        for (size_t j = j1; j < j2; j++) {

            for (size_t k = k1; k < k2; k++) {
                c(j, k) = 0;
                for (size_t t = 0; t < n; t++) {
                    c(j, k) += a(j, t) * b(t, k);
                }
            }
        }
    }
    return c;
}

/* Winograd algorithm without bandwidth management
 * https://en.wikipedia.org/wiki/Computational_complexity_of_matrix_multiplication
 * https://en.wikipedia.org/wiki/Matrix_multiplication_algorithm
 * A band matrix with Bandwidth=0 is a diagonal matrix
 * faster than schoolbook algorithm
 */
/**
 * @brief multiplyMatrix_Winograd compute the product of two matrix2D
 * with the Winograd algorithm. Full matrix
 * @param a Martrix2D
 * @param b Martrix2D
 * @return Martrix2D
 */
Matrix2D multiplyMatrix_Winograd(const Matrix2D& a, const Matrix2D& b)
{
    const size_t a_row = a.rows();
    const size_t a_col = a.cols();
    const size_t b_row = b.rows();
    const size_t b_col = b.cols();
    if (a_col != b_row)
        std::cerr << "[Function] multiplyMatrix_Winograd() a.col diff b.row";

    Matrix2D c = Matrix2D::Zero(a_row, b_col);

    for (size_t j = 0; j < a_row; j++) {
      for (size_t k = 0; k < b_col; k++) {
          c(j, k) = 0;
          for (size_t t = 0; t < b_row; t++) {
              c(j,k) += a(j, t) * b(t, k);
          }
      }
    }

    return c;

 }

/**
 * @brief inverseMatSym0 Cette procedure execute l'inversion d'une matrice en utilisant la
 * formule de la comatrice
 * @param matrix
 * @param shift
 * @return
 */
Matrix2D inverseMatSym0(const Matrix2D& matrix, const size_t shift)
{
    if (matrix.rows() != matrix.cols())
        throw std::runtime_error("Matrix is not quadratic");


    const Matrix2D& matrix2 = seedMatrix(matrix, shift);

    //const size_t n = matrix.rows();
    Matrix2D matInv = matrix.inverse();//matrix.rows(), matrix.cols());

   /* if (n == 1) {
        matInv(0, 0) = 1.0L / matrix(0, 0);
        return matInv;
    }


    const Matrix2D& matInv2 = comatrice0(matrix2);

    const auto det = determinant(matrix2);
    if (det == 0) {
        std::cout << "[Function::inverseMatSym0] det == 0" << std::endl;
        throw std::runtime_error("[Function::inverseMatSym0] det == 0");
    }
    for (size_t i = shift; i < n-shift; i++)
        for (size_t j = shift; j< n-shift; j++)
            matInv(i, j) = matInv2(i-shift, j-shift) / det;
*/

    return matInv;
}


Matrix2D inverse_padded_matrix(const Matrix2D& paddedMatrix, const long shift)
{
    auto n = paddedMatrix.rows() - 2 *shift;
    auto m = paddedMatrix.cols() - 2 *shift;
#ifdef DEBUG
    if (n != m)
        throw std::runtime_error("[inverse_padded_matrix] Matrix is not quadratic");
#endif

    // Crée une copie pour le résultat
    Matrix2D inverseMatrix = Matrix2D::Zero(paddedMatrix.rows(), paddedMatrix.cols());

    // Extraction du bloc central
    Matrix2D centralBlock = paddedMatrix.block(shift, shift, n, m);

    // Vérification du conditionnement (optionnelle mais utile en debug)
#ifdef DEBUG
    Eigen::JacobiSVD<Matrix2D> svd(centralBlock);
    double cond = svd.singularValues()(0) / svd.singularValues().tail(1)(0);
    if (!std::isfinite(cond) || cond > 1e10)
        std::cerr << "[inverse_padded_matrix] Warning: poorly conditioned matrix (cond=" << cond << ")\n";
#endif
    // Définir un bloc de la matrice (partie centrale)
    //Matrix2D inverseMatrix = paddedMatrix;
    //Eigen::Block<Matrix2D> centralBlock = inverseMatrix.block(shift, shift, n, m);


    // Inversion avec gestion des petits cas
    Matrix2D centralInverse;
    if (centralBlock.rows() < 20) {
        centralInverse = centralBlock.inverse(); // Inverser et stocker dans inverseMatrix

    } else {
        // Décomposition LU
        Eigen::PartialPivLU<Matrix2D> lu(centralBlock);

        // Inverser la matrice

#ifdef DEBUG
        Eigen::JacobiSVD<Matrix2D> svd(centralBlock);
        double cond = svd.singularValues()(0) / svd.singularValues().tail(1)(0);
        if (cond > 1e12 || !std::isfinite(cond)) {
            throw std::runtime_error("[inverse_padded_matrix] Matrix is ill-conditioned or singular.");
        } else {
            centralInverse = lu.inverse();
        }

#else
        centralInverse = lu.inverse(); // Inverser et stocker dans inverseMatrix
#endif
    }

    // Placement dans la matrice padded
    inverseMatrix.block(shift, shift, n, m) = centralInverse;

    return inverseMatrix; // Retourner la matrice inversée
}

/**
 * @details Generalizd Moore-Penrose inverse
 * @ref  https://arxiv.org/pdf/0804.4809v1
 */
Eigen::MatrixXd geninv(const Eigen::MatrixXd& G)
{
    int m = G.rows();
    int n = G.cols();
    bool transpose = false;

    Eigen::MatrixXd A;
    if (m < n) {
        transpose = true;
        A = G * G.transpose();
        n = m;
    } else {
        A = G.transpose() * G;
    }

    Eigen::VectorXd dA = A.diagonal();
    double tol = dA(dA.array() > 0).minCoeff() * 1e-9;

    Eigen::MatrixXd L = Eigen::MatrixXd::Zero(A.rows(), A.cols());
    int r = 0;

    for (int k = 0; k < n; ++k) {
        ++r;
        Eigen::VectorXd vec = A.block(k, k, n - k, 1);

        if (r > 1) {
            Eigen::MatrixXd L_sub = L.block(k, 0, n - k, r - 1);
            Eigen::RowVectorXd Lk = L.block(k, 0, 1, r - 1);
            vec -= L_sub * Lk.transpose();
        }

        if (vec(0) > tol) {
            L(k, r - 1) = std::sqrt(vec(0));
            if (k + 1 < n) {
                L.block(k + 1, r - 1, n - k - 1, 1) = vec.segment(1, n - k - 1) / L(k, r - 1);
            }
        } else {
            --r;
        }
    }

    L = L.leftCols(r);
    Eigen::MatrixXd M = (L.transpose() * L).inverse();

    if (transpose) {
        return G.transpose() * L * M * M * L.transpose();
    } else {
        return L * M * M * L.transpose() * G.transpose();
    }
}

Matrix2D geninv(const Matrix2D& G) {
    long m = G.rows();
    long n = G.cols();
    bool transpose = false;

    Matrix2D A;
    if (m < n) {
        transpose = true;
        A = G * G.transpose();
        n = m;

    } else {
        A = G.transpose() * G;
    }

    Matrix2D dA = A.diagonal();
    //t_matrix tol = dA(dA.array() > 0).minCoeff() * 1e-9;


    std::vector<t_matrix> positive_diagonal;
    for (long i = 0; i < A.rows(); ++i) {
        t_matrix val = A(i, i);
        if (val > 0)
            positive_diagonal.push_back(val);
    }

    t_matrix tol;
    if (!positive_diagonal.empty()) {
        tol = *std::min_element(positive_diagonal.begin(), positive_diagonal.end()) * 1e-9L;
    } else {
        tol = std::numeric_limits<t_matrix>::epsilon();
    }



    Matrix2D L = Matrix2D::Zero(A.rows(), A.cols());
    long r = 0;

    for (long k = 0; k < n; ++k) {
        ++r;
        Matrix2D vec = A.block(k, k, n - k, 1);

        if (r > 1) {
            Matrix2D L_sub = L.block(k, 0, n - k, r - 1);
            RowVectorLD Lk = L.block(k, 0, 1, r - 1);
            vec -= L_sub * Lk.transpose();
        }

        if (vec(0) > tol) {
            L(k, r - 1) = std::sqrt(vec(0));
            if (k + 1 < n) {
                L.block(k + 1, r - 1, n - k - 1, 1) = vec.block(1, 0, n - k - 1, 1) / L(k, r - 1);
            }

        } else {
            --r;
        }
    }

    L = L.leftCols(r);
    Matrix2D M = (L.transpose() * L).inverse();

    if (transpose) {
        return G.transpose() * L * M * M * L.transpose();

    } else {
        return L * M * M * L.transpose() * G.transpose();
    }
}

Eigen::MatrixXd pseudoInverseSVD(const Eigen::MatrixXd& G, double tol)
{
    using namespace Eigen;
    JacobiSVD<MatrixXd> svd(G, ComputeThinU | ComputeThinV);
    const auto& S = svd.singularValues();
    MatrixXd S_inv = MatrixXd::Zero(svd.matrixV().cols(), svd.matrixU().cols());

    for (int i = 0; i < S.size(); ++i)
        if (S(i) > tol)
            S_inv(i, i) = 1.0 / S(i);

    return svd.matrixV() * S_inv * svd.matrixU().transpose();
}

Matrix2D pseudoInverseSVD(const Matrix2D& G, t_matrix tol)
{
    JacobiSVD<Matrix2D> svd(G, ComputeThinU | ComputeThinV);
    const auto& S = svd.singularValues();
    Matrix2D S_inv = Matrix2D::Zero(svd.matrixV().cols(), svd.matrixU().cols());

    for (int i = 0; i < S.size(); ++i)
        if (S(i) > tol)
            S_inv(i, i) = 1.0 / S(i);

    return svd.matrixV() * S_inv * svd.matrixU().transpose();
}


Matrix2D inverse_banded_padded_matrix(const Matrix2D& bandedPaddedMatrix, const long shift)
{

    auto n = bandedPaddedMatrix.rows();
    auto m = bandedPaddedMatrix.cols() - 2 *shift;


    // Crée une copie pour le résultat
    Matrix2D inverseMatrix = Matrix2D::Zero(bandedPaddedMatrix.rows(), bandedPaddedMatrix.cols());

    // Extraction du bloc central
    Matrix2D centralBlock = bandedPaddedMatrix.block(0, shift, n, m);
   // showMatrix(centralBlock, "centrale");

    // Inversion avec gestion des petits cas
    Matrix2D centralInverse;
    centralInverse = geninv(centralBlock);

    //Matrix2D centralInverse2 = pseudoInverseSVD(centralBlock);

   //showMatrix(centralInverse2 * centralBlock, "Controle pseudo SVD");


    // Placement dans la matrice padded
    inverseMatrix.block(shift, 0, m, n) = centralInverse;
    //showMatrix(centralInverse * centralBlock, "Controle 1");

    return inverseMatrix; // Retourner la matrice inversée
}


t_matrix determinant_padded_matrix(const Matrix2D& paddedMatrix, const long shift)
{
    auto n = paddedMatrix.rows() - 2 *shift;
    auto m = paddedMatrix.cols() - 2 *shift;
#ifdef DEBUG
    if (n != m)
        throw std::runtime_error("[determinant_padded_matrix] Matrix is not quadratic");
#endif

    t_matrix determinant;
    // Définir un bloc de la matrice (partie centrale)
    Matrix2D inverseMatrix = paddedMatrix;
    Eigen::Block<Matrix2D> centralBlock = inverseMatrix.block(shift, shift, n, m);

    if (centralBlock.rows() < 2) {
        determinant = centralBlock.determinant();

    } else {
        // Décomposition LU // pour des matrices générales
       /* Eigen::PartialPivLU<Matrix2D> lu(centralBlock);
          determinant = lu.determinant();
        */

        Eigen::LDLT<Matrix2D> ldlt(centralBlock); // pour des matrices SPD
        //determinant = ldlt.vectorD().array().log().sum().exp();  // produit des Dᵢ
        t_matrix log_det = ldlt.vectorD().array().log().sum(); // Pour éviter le dépassement de capacité ou les problèmes de précision
        determinant  = std::exp(log_det);

    }
    return determinant;
}



t_matrix ln_rate_determinant_padded_matrix_A_B(const Matrix2D& paddedMatrix_A,
                                               const Matrix2D& paddedMatrix_B,
                                               const long shift)
{
    auto na = paddedMatrix_A.rows() - 2 * shift;
    auto ma = paddedMatrix_A.cols() - 2 * shift;
#ifdef DEBUG
    if (na != ma)
        throw std::runtime_error("[inverse_padded_matrix] Matrix A is not quadratic");
#endif
    t_matrix log_det_A = paddedMatrix_A.block(shift, shift, na, ma)
                             .ldlt()
                             .vectorD()
                             .array()
                             .log()
                             .sum();

    auto nb = paddedMatrix_B.rows() - 2 * shift;
    auto mb = paddedMatrix_B.cols() - 2 * shift;
#ifdef DEBUG
    if (nb != mb)
        throw std::runtime_error("[inverse_padded_matrix] Matrix B is not quadratic");
#endif
    t_matrix log_det_B = paddedMatrix_B.block(shift, shift, nb, mb)
                             .ldlt()
                             .vectorD()
                             .array()
                             .log()
                             .sum();

    return log_det_A - log_det_B;
}


/**
 * Inversion d'une matrice symétrique définie positive
 * à partir de sa décomposition de Cholesky (L, D)
 * selon l'algorithme de Hutchison et De Hoog (1985)
 *
 * La matrice d'origine A peut être reconstruite comme: A = L * D * L^T
 * où L est une matrice triangulaire inférieure avec des 1 sur la diagonale
 * et D est une matrice diagonale avec des valeurs positives
 *
 * @param decomp Une paire contenant la matrice L et le vecteur diagonal D
 * @return La matrice inverse de A
 */
Matrix2D choleskyInvert(const std::pair<Matrix2D, DiagonalMatrixLD>& decomp)
{
    const Matrix2D &L = decomp.first;    // Matrice triangulaire inférieure
    const DiagonalMatrixLD& D = decomp.second; // Vecteur diagonal
    size_t n = D.rows();                 // Dimension de la matrice

    // Initialiser la matrice inverse avec des zéros
    Matrix2D inv = Matrix2D::Zero(n, n);

    // Étape 1: Calculer Z = L^(-1)
    Matrix2D Z = Matrix2D::Zero (n, n);
    for (size_t i = 0; i < n; i++) {
        Z(i, i) = 1.0; // Diagonale = 1

        // Calculer les éléments sous la diagonale
        for (size_t j = i + 1; j < n; j++) {
            t_matrix sum = 0.0;
            for (size_t k = i; k < j; k++) {
                sum -= L(j, k) * Z(k, i);
            }
            Z(j, i) = sum;
        }
    }

    // Étape 2: Calculer W = D^(-1) * Z^T
    Matrix2D W = Matrix2D::Zero (n, n);
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j <= i; j++) {
            W(j, i) = Z(i, j) / D.diagonal()[i];
        }
    }

    // Étape 3: Calculer inv = Z * W = Z * D^(-1) * Z^T
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j <= i; j++) {
            t_matrix sum = 0.0;
            for (size_t k = i; k < n; k++) {
                sum += Z(k, i) * W(j, k);
            }
            inv(i, j) = sum;
            inv(j, i) = sum; // Symétrie
        }
    }

    return inv;
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
Matrix2D inverseMatSym_origin(const std::pair<Matrix2D, DiagonalMatrixLD> &decomp, const size_t nbBandes, const size_t shift)
{
    const Matrix2D &L = decomp.first;
    const DiagonalMatrixLD& D = decomp.second;
    size_t dim = L.rows();
    Matrix2D matInv = Matrix2D::Zero (dim, dim);
    size_t bande = floor((nbBandes-1)/2);

    matInv(dim-1-shift, dim-1-shift) = 1.0 / D.diagonal()[dim-1-shift];

    if (dim >= 4) {
        matInv(dim-2-shift, dim-1-shift) = -L(dim-1-shift, dim-2-shift) * matInv(dim-1-shift, dim-1-shift);
        matInv(dim-2-shift, dim-2-shift) = (1. / D.diagonal()[dim-2-shift]) - L(dim-1-shift, dim-2-shift) * matInv(dim-2-shift, dim-1-shift);
    }

    // shift : décalage qui permet d'éliminer les premières et dernières lignes et colonnes
    // La boucle suivante n'est executée que si dim >=5
    if (dim >= 5) {
        for (size_t i = dim-3-shift; i>=shift; --i) {
            matInv(i, i+2) = -L(i+1, i) * matInv(i+1, i+2) - L(i+2, i) * matInv(i+2, i+2);
            matInv(i, i+1) = -L(i+1, i) * matInv(i+1, i+1) - L(i+2, i) * matInv(i+1, i+2);
            matInv(i, i) = (1.0 / D.diagonal()[i]) - L(i+1, i) * matInv(i, i+1) - L(i+2, i) * matInv(i, i+2);
            
            if (bande >= 3)  {
                for (size_t k=3; k<=bande; ++k) {
                    if (i+k < (dim - shift))  {
                        matInv(i, i+k) = -L(i+1, i) * matInv(i+1, i+k) - L(i+2, i) * matInv(i+2, i+k);
                    }
                }
            }
        }
    }
    
    for (size_t i = shift; i< dim-shift; ++i)  {
        for (size_t j = i+1; j <= i+bande; ++j)  {
            if (j < (dim-shift))   {
                matInv(j, i) = matInv(i, j);
            }
        }
    }

    return matInv;
}



Matrix2D inverseMatSym(const Matrix2D& matrixLE, const DiagonalMatrixLD &matrixDE, const int nbBandes, const size_t shift)
{
    const size_t dim = matrixLE.rows();
    Matrix2D matInv = Matrix2D::Zero (dim, dim);
    const size_t bande = floor((nbBandes-1)/2);

    matInv(dim-1-shift, dim-1-shift) = 1.0 / matrixDE.diagonal()[dim-1-shift];
    matInv(dim-2-shift, dim-1-shift) = -matrixLE(dim-1-shift, dim-2-shift) * matInv(dim-1-shift, dim-1-shift);
    matInv(dim-2-shift, dim-2-shift) = (1.0 / matrixDE.diagonal()[dim-2-shift]) - matrixLE(dim-1-shift, dim-2-shift) * matInv(dim-2-shift, dim-1-shift);

    if (bande >= 3) {
        for (size_t i = dim-3-shift; i >= shift; --i) {
                matInv(i, i+2) = -matrixLE(i+1, i) * matInv(i+1, i+2) - matrixLE(i+2, i) * matInv(i+2, i+2);
                matInv(i, i+1) = -matrixLE(i+1, i) * matInv(i+1, i+1) - matrixLE(i+2, i) * matInv(i+1, i+2);
                matInv(i, i) = (1.0 / matrixDE.diagonal()[i]) - matrixLE(i+1, i) * matInv(i, i+1) - matrixLE(i+2, i) * matInv(i, i+1);

                for (size_t k = 3; k <= bande; ++k) {
                    if (i+k < (dim - shift)) {
                        matInv(i, i+k) = -matrixLE(i+1, i) * matInv(i+1, i+k) - matrixLE(i+2, i) * matInv(i+2, i+k);
                    }//else What we do?
                }
        }

     } else {
        for (size_t i = dim-3-shift; i >= shift; --i) {
            matInv(i, i+2) = -matrixLE(i+1, i) * matInv(i+1, i+2) - matrixLE(i+2, i) * matInv(i+2, i+2);
            matInv(i, i+1) = -matrixLE(i+1, i) * matInv(i+1, i+1) - matrixLE(i+2, i) * matInv(i+1, i+2);
            matInv(i, i) = (1. / matrixDE.diagonal()[i]) - matrixLE(i+1, i) * matInv(i, i+1) - matrixLE(i+2, i) * matInv(i, i+2);

            for (size_t k = 3; k <= bande; ++k) {
                if (i+k <= (dim-1 - shift)) {
                    matInv(i, i+k) = -matrixLE(i+1, i) * matInv(i+1, i+k) - matrixLE(i+2, i) * matInv(i+2, i+k);
                }//else What we do?
            }
        }
     }

    /* Code RenCurve
     * for i:=(dim-2-dc) downto 1+dc do begin
          Mat_1(i,i+2):= -Mat_L_E(i+1,i)*Mat_1(i+1,i+2) - Mat_L_E(i+2,i)*Mat_1(i+2,i+2);
          Mat_1(i,i+1):= -Mat_L_E(i+1,i)*Mat_1(i+1,i+1) - Mat_L_E(i+2,i)*Mat_1(i+1,i+2);
          Mat_1(i,i)  := (1/Mat_D_E(i,i)) - Mat_L_E(i+1,i)*Mat_1(i,i+1) - Mat_L_E(i+2,i)*Mat_1(i,i+2);
          if (bande>=3) then begin
             for k:=3 to bande do begin
                if (i+k<=(dim-dc)) then begin
                   Mat_1(i,i+k):= -Mat_L_E(i+1,i)*Mat_1(i+1,i+k) - Mat_L_E(i+2,i)*Mat_1(i+2,i+k);
                end;
             end;
          end;
       end;
     */

    //On symétrise la matrice Mat_1, même si cela n'est pas nécessaire lorsque bande=2
    for (size_t i = shift; i <= (dim - 1 -shift); ++i) {
        for (size_t j = i+1; j <= i+bande; ++j) {
            if (j <= (dim - 1 -shift)) {
                matInv(j, i) = matInv(i, j);
            } //else What we do?
        }
    }


  /*  for (int i = shift; i < dim-shift-1; ++i) {
        for (int j = i+1; j <= std::min(i+bande, dim-shift-1) ; ++j) {
                matInv(j, i) = matInv.at(i).at(j);
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
    for (size_t i = 0; i < matrix.size(); ++i) {
        sum += std::accumulate(matrix.at(i).begin(), matrix.at(i).end(), 0.);
    }
    return sum;
}

t_matrix sumAllMatrix(const Matrix2D &m)
{
    t_matrix som = 0;
    size_t n = m.rows();
    for (size_t j = 0; j < n; ++j)
        for (size_t k = 0; k < n; ++k)
            som += m(j, k);

    return som;
}




/* Never used !!!
 */
double sumAllVector(const std::vector<double>& vector)
{
    return std::accumulate(vector.begin(), vector.end(), 0.);
}

Matrix2D cofactor0(const Matrix2D& matrix)
{
    const size_t n = matrix.rows();
    Matrix2D result (n, n);
    Matrix2D matMinorTmp (n-1, n-1);
    t_matrix det;
    size_t i1, k1;
    for (size_t j=0; j<n; j++) {
        for (size_t i=0; i<n; i++) {
            i1 = 0;
            for (size_t k=0; k<n; k++) {
                if (k == i)
                    continue;
                k1 = 0;
                for (size_t l=0; l<n; l++) {
                    if (l == j)
                        continue;
                    matMinorTmp(i1, k1) = matrix(k, l);
                    k1++;
                }
                i1++;
            }

            det = determinant(matMinorTmp);

            result(i,j ) = pow(-1.0, i+j+2.) * det;
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
    const size_t n = matrix.rows();
    Matrix2D result = Matrix2D::Zero (n, n);
    Matrix2D matMinorTmp = Matrix2D::Zero (n-1, n-1);
    t_matrix det;
    size_t i1, k1;
    for (size_t j=0; j<n; j++) {
        for (size_t i=0; i<n; i++) {
            i1 = 0;
            for (size_t k=0; k<n; k++) {
                if (k == i)
                    continue;
                k1 = 0;
                for (size_t l=0; l<n; l++) {
                    if (l == j)
                        continue;
                    matMinorTmp(i1, k1) = matrix(k, l);
                    k1++;
                }
                i1++;
            }

            det = determinant(matMinorTmp);

            result(j, i) = pow(-1., i+j+2.) * det;
        }
    }
    return result;
}

/**
 * @brief choleskyLL0 décompose une matrice en triange inférieur et supérieur tel que L Lt
 * @param matrix, la matrice doit être strictement positive. Sinon utiliser la version MoreSorensen
 * @return matrix triangulaire supérieur
 */

Matrix2D choleskyLL0(const Matrix2D &matrix)
{
    const size_t n = matrix.rows();

    Matrix2D L = Matrix2D::Zero (n, n);
    t_matrix sum;

    for (size_t i = 0; i < n; i++) {
        sum = matrix(i,  i);
        for (size_t k = 0; k < i; k++)
            sum -= pow(L(i, k), 2.);

        L(i, i) = sqrt(sum);

        for (size_t j = i+1; j < n; j++) {
            sum = matrix(j, i);
            for (size_t k = 0; k < i; k++)
                sum -= L(i, k) * L(j, k);

            L(j, i) = sum / L(i, i);
        }
    }

    return L;
}


void verify_cholesky_decomposition(const Matrix2D& original, const Matrix2D& L, bool verbose = true) {
    // Reconstruction
    Matrix2D reconstructed = L * L.transpose();
    Matrix2D error_matrix = original - reconstructed;
    std::cout << "original \n" << original <<std::endl;
    std::cout << "reconstructed \n" << reconstructed <<std::endl;
    std::cout << "error_matrix \n" << error_matrix <<std::endl;

    // Métriques d'erreur
    struct ErrorMetrics {
        double frobenius_norm;      // Norme de Frobenius
        double relative_error;      // Erreur relative
        double max_absolute_error;  // Erreur absolue maximale
        double condition_number;    // Conditionnement (si nécessaire)
    };

    ErrorMetrics metrics;
    metrics.frobenius_norm = error_matrix.norm();
    metrics.relative_error = metrics.frobenius_norm / original.norm();
    metrics.max_absolute_error = error_matrix.cwiseAbs().maxCoeff();

    // Calcul du conditionnement (optionnel, plus coûteux)
    Eigen::JacobiSVD<Matrix2D> svd(original);
    auto singular_values = svd.singularValues();
    metrics.condition_number = singular_values.maxCoeff() / singular_values.minCoeff();

    if (verbose) {
        std::cout << "[Cholesky Verification]\n";
        std::cout << "  Norme de Frobenius de l'erreur: " << metrics.frobenius_norm << "\n";
        std::cout << "  Erreur relative: " << metrics.relative_error << "\n";
        std::cout << "  Erreur absolue max: " << metrics.max_absolute_error << "\n";
        std::cout << "  Conditionnement: " << metrics.condition_number << "\n";

        // Interprétation des résultats
        if (metrics.relative_error < 1e-12) {
            std::cout << "  ✓ Excellente précision\n";

        } else if (metrics.relative_error < 1e-9) {
            std::cout << "  ✓ Bonne précision\n";

        } else if (metrics.relative_error < 1e-6) {
            std::cout << "  ⚠ Précision acceptable\n";

        } else {
            std::cout << "  ✗ Précision insuffisante\n";
        }

        if (metrics.condition_number > 1e12) {
            std::cout << "  ⚠ Matrice mal conditionnée\n";
        }
    }
}

// code qui fonctionne mais pourquoi ???
Matrix2D cholesky_LLt_MoreSorensen(const Matrix2D &matrix)
{


   /* Pour test
    * t_matrix data[] = {
        4, 2, 2, 0, 0, 0,
        2, 4, 2, 0, 0, 0,
        2, 2, 4, 2, 0, 0,
        0, 0, 2, 4, 2, 2,
        0, 0, 0, 2, 4, 2,
        0, 0, 0, 2, 2, 4
    };
    // Utiliser Eigen::Map pour créer une matrice à partir du tableau
    Eigen::Map<Matrix2D> matrix2(data, 6, 6);
    * resultat : L=
    *  2        0        0        0        0        0
       1  1.73205        0        0        0        0
       1  0.57735  1.63299        0        0        0
       0        0  1.22474  1.58114        0        0
       0        0        0  1.26491  1.54919        0
       0        0        0  1.26491 0.258199  1.52753

    */

    const size_t n = matrix.rows();
    Matrix2D L = Matrix2D::Zero (n, n);
#ifdef DEBUG
    bool MS = false;
#endif
    // Paramètre pour la correction
    const t_matrix delta = 1e-20;

    for (size_t j = 0; j < n; ++j) {
        // Calcul des éléments diagonaux L[j][j]

        t_matrix sum = matrix(j, j);
        for (size_t k = 0; k < j; ++k) {
            sum -= L(j, k) * L(j, k); // Utilisation de la multiplication au lieu de pow pour plus d'efficacité
        }

        // Correction de Moré et Sorensen

        if (sum <= delta) {
#ifdef DEBUG
            MS = true;
#endif
            sum = std::max(delta, std::abs(sum)); // Assurer une valeur positive pour la racine carrée, en utlisant abs(sum) on reste dans les grandeurs d'ordres

        }

        L(j, j) = sqrt(sum);

        // Calcul des éléments hors diagonale
        for (size_t i = j + 1; i < n; ++i) {
            t_matrix sumOffDiag = matrix(i, j);
            for (size_t k = 0; k <= j; ++k) {
                sumOffDiag -= L(i, k) * L(j, k);
            }
            L(i, j) = sumOffDiag / L(j, j);
        }
    }


#ifdef DEBUG
    if (MS) {
        std::cout << "[Function::cholesky_LLt_MoreSorensen]  MS = true\n\n ";
        verify_cholesky_decomposition(matrix, L);
    }
#endif


    // test
    /* ✅ Solution recommandée : utiliser LDLt avec tolérance
     * LDLt fonctionne même pour les matrices semi-définies positives, tant qu'elles sont symétriques
     * Ne Produit pas une précision meilleur à notre choleskyLDLT sans pivot
     */
   /* Eigen::LDLT<Matrix2D> ldlt(matrix);
    if (ldlt.info() != Eigen::Success) {
        std::cerr << "LDLT decomposition failed" << std::endl;
    }
    // L = ldlt.matrixL();
    // D = ldlt.vectorD().asDiagonal();

    auto L1 = ldlt.matrixL();
    showMatrix(L1, "L1 ldlt");
    auto Dvec = ldlt.vectorD();
    showMatrix(Dvec, " Dvec");

    Matrix2D Lmod = L1;

    for (int i = 0; i < Dvec.size(); ++i) {
        t_matrix sqrtD = (Dvec[i] > 0) ? std::sqrt(Dvec[i]) : 0.0;  // tolérance ici
        Lmod.col(i) *= sqrtD;
    }
    showMatrix(Lmod, "Lmod") ;
    verify_cholesky_decomposition(matrix, Lmod);
*/

    // À ce stade : matrix ≈ Lmod * Lmod.transpose()
    /* ✅ Variante robuste : pseudo-Cholesky (valeurs propres ≥ 0)
     * Si tu veux éviter les NaNs même si la matrice a de très petites valeurs propres négatives dues au bruit numérique, tu peux :
     * Forcer les valeurs propres négatives à zéro (ou très petit ε)
     * Recomposer manuellement une racine semi-positive
     * NE DONNE PAS UNE MATRICE TRIANGULAIRE
    */
    /* Eigen::SelfAdjointEigenSolver<Matrix2D> eig(matrix2);
    auto D2 = eig.eigenvalues();
    auto Q = eig.eigenvectors();

    for (int i = 0; i < D2.size(); ++i)
        D2[i] = std::max(0.0L, D2[i]);  // tolérance

    Matrix2D L2 = Q * D2.cwiseSqrt().asDiagonal();
   showMatrix(L2, "L2") ;
    verify_cholesky_decomposition(matrix2, L2);
*/

    return L;
}





/** Ne marche pas toujours
 * @brief Delta adaptatif : Plutôt qu'un delta fixe, implémentation d'une méthode adaptative qui :
 * - Calcule une valeur initiale basée sur la norme de la matrice
 * - Augmente progressivement delta par un facteur beta jusqu'à ce que la correction soit suffisante.
 *
 * @param matrix
 * @return
 */
Matrix2D cholesky_LLt_MoreSorensen_adapt(const Matrix2D& matrix)
{
    const size_t n = matrix.rows();
#ifdef DEBUG
    // Vérification que la matrice est carrée
    for (size_t i = 0; i < n; ++i) {
        if (matrix.cols() != matrix.rows()) {
            throw std::invalid_argument("La matrice doit être carrée pour la décomposition de Cholesky");
        }
    }
    bool MS = false;
#endif

    // Faire une copie de la matrice originale pour ne pas la modifier
    Matrix2D A = matrix;
    Matrix2D L = Matrix2D::Zero (n, n);

    // Paramètres pour la correction
    const t_matrix delta_min = 1e-20;  // Delta minimum
    t_matrix delta = 0.0;             // Delta adaptatif
    const t_matrix beta = 2;        // Facteur multiplicatif


    for (size_t j = 0; j < n; ++j) {
        // Calcul de l'élément diagonal L[j][j]
        t_matrix sum = A(j, j);
        for (size_t k = 0; k < j; ++k) {
            sum -= L(j, k) * L(j, k);
        }

        // Application de la correction de Moré et Sorensen si nécessaire
        if (sum <= delta_min) {

            // Calcul de la norme maximale pour estimation initiale de delta
            t_matrix max_diag = 0.0;
            t_matrix max_offdiag = 0.0;
            for (size_t i = 0; i < n; ++i) {
                max_diag = std::max(max_diag, std::abs(A(i, i)));
                for (size_t j = 0; j < i; ++j) {
                    max_offdiag = std::max(max_offdiag, std::abs(A(i, j)));
                }
            }

            // Valeur initiale pour delta basée sur la norme de la matrice
            t_matrix initial_delta = std::max(delta_min, delta_min * std::max(max_diag, max_offdiag));



#ifdef DEBUG
            MS = true;
#endif
            // Calcul du delta adaptatif
            if (delta < initial_delta) {
                delta = initial_delta;
            }

            // Augmenter delta jusqu'à ce que sum + delta soit positif
            while (sum + delta <= delta_min) {
                delta *= beta;
            }

            // Appliquer delta à tous les éléments diagonaux restants
            for (size_t k = j; k < n; ++k) {
                A(k, k) += delta;
            }

            // Recalculer sum avec le delta ajouté
            sum = A(j, j);
            for (size_t k = 0; k < j; ++k) {
                sum -= L(j, k) * L(j, k);
            }
        }

        L(j, j) = sqrt(sum);

        // Calcul des éléments sous-diagonaux (L[i][j] pour i > j)
        for (size_t i = j + 1; i < n; ++i) {
            t_matrix sumOffDiag = A(i, j);
            for (size_t k = 0; k < j; ++k) {
                sumOffDiag -= L(i, k) * L(j, k);
            }
            L(i, j) = sumOffDiag / L(j,j);
        }
    }

    // Mise à zéro des éléments au-dessus de la diagonale
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            L(i, j) = 0.0;
        }
    }

#ifdef DEBUG
    if (MS) {
        Matrix2D Lt = transpose0(L);
        Matrix2D prod = multiMatParMat0(L, Lt);
        // test la différence entrée sortie
        t_matrix som_prod = 0;
        for (size_t i = 0; i < n; i++)
            for (size_t j = 0; j < n; j++)
                som_prod += prod(i, j);

        t_matrix som_matrix = 0;
        for (size_t i = 0; i < n; i++)
            for (size_t j = 0; j < n; j++)
                som_matrix += matrix(i, j);

        std::cout << "[Function::cholesky_LLt_MoreSorensen] diff MS = "
                  << static_cast<double>(som_prod - som_matrix) << std::endl;
        std::cout << "[Function::cholesky_LLt_MoreSorensen] La matrice n'est pas définie positive. "
                  << "Somme produit: " << static_cast<double>(som_prod)
                  << " Somme matrice originale: " << static_cast<double>(som_matrix)
                  << " Delta final: " << static_cast<double>(delta) << std::endl;
        //showMatrix(prod);
        //showMatrix(matrix);

    }
#endif

    return L;
}

/**
 * @brief choleskyLDLt
 * @param matrix
 * @return pair of 2 matrix
 */

/*
std::pair<Matrix2D, DiagonalMatrixLD> choleskyLDLT(const Matrix2D& matrix)
{

#ifdef DEBUG
    if (matrix.rows() != matrix.cols()) {
        std::cout << "[Function::choleskyLDLT] Matrix must be square";
        throw std::invalid_argument("[Function::choleskyLDLT] Matrix must be square");
    }
#endif
    const size_t n = matrix.rows();
    Matrix2D L = Matrix2D::Identity(n, n);  // ✅ Identity plus efficace que Zero + boucle
    DiagonalMatrixLD D(n);

    for (size_t i = 0; i < n; i++) {
        // ✅ ÉTAPE 1: Calculer D[i] EN PREMIER
        D.diagonal()[i] = matrix(i, i);
        for (size_t k = 0; k < i; k++) {
            D.diagonal()[i] -= L(i, k) * L(i, k) * D.diagonal()[k];
        }

        // ✅ Vérification de la stabilité numérique
        if (D.diagonal()[i] <= 0) {
            std::cout << "[Function::choleskyLDLT] Matrix is not positive definite";
            //throw std::runtime_error("[Function::choleskyLDLT] Matrix is not positive definite");
            return std::make_pair(L, D);
        }

        // ✅ ÉTAPE 2: Calculer L[j,i] pour j > i (colonne i de L)
        for (size_t j = i + 1; j < n; j++) {
            L(j, i) = matrix(j, i);  // ✅ Utiliser matrix(j,i) pas matrix(i,j)
            for (size_t k = 0; k < i; k++) {
                L(j, i) -= L(j, k) * L(i, k) * D.diagonal()[k];
            }
            L(j, i) /= D.diagonal()[i];  // ✅ Diviser par D[i] qui vient d'être calculé
        }
    }

    return std::make_pair(L, D);  // ✅ make_pair plus lisible

}
*/

std::pair<Matrix2D, DiagonalMatrixLD> choleskyLDLT(const Matrix2D& matrix)
{
    constexpr double epsilon = 1e-12;  // tolérance numérique

#ifdef DEBUG
    if (matrix.rows() != matrix.cols()) {
        std::cout << "[Function::choleskyLDLT] Matrix must be square\n";
        throw std::invalid_argument("Matrix must be square");
    }
#endif

    const size_t n = matrix.rows();
    Matrix2D L = Matrix2D::Identity(n, n);  // plus sûr
    DiagonalMatrixLD D(n);

    for (size_t i = 0; i < n; ++i) {
        double sum = matrix(i, i);
        for (size_t k = 0; k < i; ++k) {
            sum -= L(i, k) * L(i, k) * D.diagonal()[k];
        }

        // Vérifie la stabilité numérique
        if (sum < -epsilon) {
            std::cout << "[Function::choleskyLDLT] Matrix is not positive semi-definite at row " << i << "\n";
            D.diagonal()[i] = 0.0;

        } else if (std::abs(sum) < epsilon) {
            D.diagonal()[i] = 0.0;  // tolère les petits pivots nuls

        } else {
            D.diagonal()[i] = sum;
        }

        // Remplir colonne i de L
        for (size_t j = i + 1; j < n; ++j) {
            double lij = matrix(j, i);
            for (size_t k = 0; k < i; ++k) {
                lij -= L(j, k) * L(i, k) * D.diagonal()[k];
            }

            // Évite division par zéro
            if (std::abs(D.diagonal()[i]) > epsilon)
                L(j, i) = lij / D.diagonal()[i];

            else
                L(j, i) = 0.0;  // pas définie si D[i] == 0
        }
    }

    return {L, D};
}

// Cette fonction ne sort pas lorsqu'il y a des valeurs sur la diagonal, nulle ou inférieur à zéro
// utilise dans multisampling
std::pair<Matrix2D, DiagonalMatrixLD > choleskyLDLT(const Matrix2D &matrix, const size_t shift)
{
    const size_t n = matrix.rows();
 
    Matrix2D L = Matrix2D::Zero (n, n);
    DiagonalMatrixLD D (n);

    for (size_t i = shift; i < n-shift; i++) {
        L(i, i) = (t_matrix) (1.0);
            for (size_t j = shift; j < std::min(i, n-shift); j++) {
            L(i, j) = matrix(i, j);
                for (size_t k = shift; k< std::min(j, n-shift); k++) {
                    L(i, j) -=  L(i, k) * L(j, k) * D.diagonal()[k];
                }
                L(i, j) /= D.diagonal()[j];
            }
            D.diagonal()[i] = matrix(i, i);
            for (size_t j = shift; j< std::min(i, n-shift); j++)
                D.diagonal()[i] -=  D.diagonal()[j] * powl(L(i,j), 2.L);
      }

    return std::pair<Matrix2D, DiagonalMatrixLD>(L, D);
}

/**
 * @brief robust_LLt
 * fonctionne même si la matrice n'est pas Symétrique Définie Positive
 * @param matrix
 * @return
 * ✅ Solution recommandée : utiliser LDLT avec tolérance
 * LDLT fonctionne même pour les matrices semi-définies positives, tant qu'elles sont symétriques
 */
Matrix2D robust_LLt(const Matrix2D& matrix)
{
    constexpr t_matrix epsilon = 1e-100;  // tolérance numérique

    //
#ifdef DEBUG
    if (matrix.rows() != matrix.cols()) {
        std::cout << "[Function::robust_LLt] Matrix must be square\n";
        throw std::invalid_argument("[Function::robust_LLt] Matrix must be square");
    }
#endif

    const size_t n = matrix.rows();
    Matrix2D L = Matrix2D::Identity(n, n);  // plus sûr
    DiagonalMatrixLD D(n);

    for (size_t i = 0; i < n; ++i) {
        double sum = matrix(i, i);
        for (size_t k = 0; k < i; ++k) {
            sum -= L(i, k) * L(i, k) * D.diagonal()[k];
        }

        // Vérifie la stabilité numérique
       /* if (sum < -epsilon) {
            std::cout << "[Function::robust_LLt] Matrix is not positive semi-definite at row " << i << "\n";
            showMatrix(matrix, "robust LLt matrix");

            D.diagonal()[i] = 0.0;

        } else if (std::abs(sum) < epsilon) {
            D.diagonal()[i] = 0.0;  // tolère les petits pivots nuls

        } else {
*/
            D.diagonal()[i] = sum;
     //   }

        // Remplir colonne i de L
        for (size_t j = i + 1; j < n; ++j) {
            double lij = matrix(j, i);
            for (size_t k = 0; k < i; ++k) {
                lij -= L(j, k) * L(i, k) * D.diagonal()[k];
            }

            // Évite division par zéro
            if (std::abs(D.diagonal()[i]) > epsilon)
                L(j, i) = lij / D.diagonal()[i];

            else
                L(j, i) = 0.0;  // pas définie si D[i] == 0
        }
    }


    //


    for (int i = 0; i < D.rows(); ++i) {
        t_matrix sqrtD = (D.diagonal()[i] > 0) ? std::sqrt(D.diagonal()[i]) : 0.0;
        L.col(i) *= sqrtD;
    }
    //showMatrix(L, " L  de robust LLt");
    //verify_cholesky_decomposition(matrix, LLmod);
    return L;
}

/**
 * @brief abs_minus Calcul std:abs(a-b) for type size_t
 * @param a
 * @param b
 * @return abs(a-b)
 */
size_t abs_minus(size_t a, size_t b)
{
    return a>b? a-b: b-a;
}

std::pair<Matrix2D, DiagonalMatrixLD > choleskyLDLT(const Matrix2D &matrix, const size_t nbBandes, const size_t shift)
{
    const size_t n = matrix.rows();
    const size_t n_shift = n>shift? n-shift: 0;//n-shift;
    Matrix2D L = Matrix2D::Zero (n, n);
    DiagonalMatrixLD D (n);

    for (size_t i = shift; i < n_shift; i++) {
        L(i, i) = 1;
            for (size_t j = shift; j < std::min(i, n_shift); j++) {
               if (abs_minus(i, j) <= nbBandes) {
                    L(i, j) = matrix(i, j);
                   for (size_t k = shift; k< std::min(j, n_shift); k++) {
                       L(i, j) -=  L(i, k) * L(j, k) *D.diagonal()[k];
                   }
                   L(i, j) /= D.diagonal()[j];
               }
            }
            D.diagonal()[i] = matrix(i, i);
            for (size_t j = shift; j< std::min(i, n_shift); j++)
                if (abs_minus(i, j) <= nbBandes)
                    D.diagonal()[i] -=  D.diagonal()[j] * pow(L(i, j), 2.);
      }

    return std::pair<Matrix2D, DiagonalMatrixLD>(L, D);
}



std::pair<Matrix2D, DiagonalMatrixLD> choleskyLDLT_Dsup0(const Matrix2D& matrix, const size_t nbBandes, const size_t shift)
{
    const size_t n = matrix.rows();
    const size_t n_shift = n-shift;
#ifdef DEBUG
    t_matrix det = determinant_gauss(matrix, shift);
    if (det == 0)
        qDebug() << "[ Function] choleskyLDLT_Dsup0 : singular matrix, not regular : determinant =" << (double) det;
#endif
    Matrix2D L = Matrix2D::Zero (n, n);
    DiagonalMatrixLD D (n);

    for (size_t i = shift; i < n_shift; i++) {
        L(i, i) = 1;
            for (size_t j = shift; j < std::min(i, n_shift); j++) {
               if (abs_minus(i, j) <= nbBandes) {
                    L(i, j) = matrix(i, j);
                   for (size_t k = shift; k< std::min(j, n_shift); k++) {
                        L(i, j) -=  L(i, k) * L(j, k) * D.diagonal()[k];
                   }
                   L(i, j) /= D.diagonal()[j];
               }
            }
            D.diagonal()[i] = matrix(i, i);
            for (size_t j = shift; j< std::min(i, n_shift); j++) {
                if (abs_minus(i, j) <= nbBandes)
                    D.diagonal()[i] -=  D.diagonal()[j] * pow(L(i, j), 2.);
            }

      }
#ifdef DEBUG
    for (size_t i = shift; i < n_shift; i++) {
        if (D.diagonal()[i] < 0) {
            qDebug() << "[ Function] choleskyLDLT_Dsup0 : D <0 change to 0"<< (double)D.diagonal()[i];
            D.diagonal()[i] = 0;
        }
    }
#endif
    return std::pair<Matrix2D, DiagonalMatrixLD>(L, D);
}


//  link to check  https://mxncalc.com/fr/cholesky-decomposition-calculator
/**
 * @brief Décomposition de Cholesky d'une matrice avec un décalage.
 *  Cf algorithme donné par S.M.Kay, "Modern Spectral estimation"  1988, page 30.
 * ISBN 13 :978-0130151599
 * @ref Kay, S. M. (dir.), 1988. Modern spectral estimation: Theory and application. Prentice-Hall signal processing series. Prentice Hall, Upper Saddle River, N.J.
 *
 *
 * Cette fonction décompose une matrice en une matrice triangulaire inférieure (matL)
 * et une matrice diagonale (matD) selon la méthode de Cholesky. La décomposition est
 * adaptée pour les matrices bandées avec un décalage donné.
 *
 * @param matrix Matrice d'entrée à décomposer bandwidth = k1+k2+1
 * @param nbBandes Nombre de bandes de la matrice.
 * @param shift Décalage utilisé pour éliminer les premières et dernières lignes et colonnes de zéros.
 *
 * @return std::pair<Matrix2D, MatrixDiag> Un paire contenant la matrice triangulaire inférieure (matL)
 *         et la matrice diagonale (matD).
 *
 * @note Cette fonction suppose que la matrice est symétrique et définie positive.
 *
 * @example
 * // Exemple d'utilisation
 * Matrix2D A = ...;
 * auto result = decompositionCholesky(A, 2, 1);
 * Matrix2D L = result.first;
 * MatrixDiag D = result.second;
 **/
std::pair<Matrix2D, DiagonalMatrixLD> decompositionCholesky(const Matrix2D &matrix, const size_t nbBandes, const size_t shift)
{
    //errno = 0;
    //if (math_errhandling & MATH_ERREXCEPT) feclearexcept(FE_ALL_EXCEPT);

    const size_t dim = matrix.rows();
    const size_t dim_shift = dim > shift ? dim-shift : 0;
    Matrix2D matL = Matrix2D::Zero (dim, dim);
    DiagonalMatrixLD matD (dim);

    if (dim - 2*shift == 1) { // cas des splines avec 3 points
        matD.diagonal()[1] = matrix(1, 1);
        matL(1, 1) = 1.0L;
        return {matL, matD};

    }

    // shift : décalage qui permet d'éliminer les premières et dernières lignes et colonnes constituées de zéro
    for (size_t i = shift; i < dim-shift; ++i) {
        matL(i, i) = 1.0L;
    }
    matD.diagonal()[shift] = matrix(shift, shift);

    try {
        for (size_t i = shift+1; i < dim_shift; ++i) {
            matL(i, shift) = matrix(i, shift) / matD.diagonal()[shift];

            // Calcul des éléments de matL avec la bande
            for (size_t j = shift+1; j < i; ++j) {
                if (abs_minus(i, j) <= nbBandes) {
                    t_matrix sum = 0.0L;
                    // Calcul de la somme des produits
                    /* for (size_t k = shift; k < j; ++k) {
                        if (abs_minus(i, k) <= nbBandes) {
                            sum += matL[i][k] * matD[k] * matL[j][k];
                        }
                    } */
                    //
                    //t_matrix sum = 0.0L;
                    t_matrix compensation = 0.0L;  // terme de compensation
                    for (size_t k = shift; k < j; ++k) {
                        if (abs_minus(i, k) <= nbBandes){
                            t_matrix product = matL(i, k) * matD.diagonal()[k] * matL(j, k);
                            // Algorithme Kahan pour la sommation
                            t_matrix y = product - compensation;
                            t_matrix t = sum + y;
                            compensation = (t - sum) - y;
                            sum = t;
                        }
                    }

                    matL(i, j) = (matrix(i, j) - sum) / matD.diagonal()[j];
                }
            }

            // Calcul de la diagonale de matD
            /*t_matrix sumDiag = 0.0L;
            for (size_t k = shift; k < i; ++k) {
                if (abs_minus(i, k) <= nbBandes) {
                    sumDiag += matL[i][k] * matL[i][k] * matD[k];
                }
            }*/
            t_matrix sumDiag = 0.0;
            t_matrix compensation = 0.0; // Compensation de Kahan

            for (size_t k = shift; k < i; ++k) {
                if (abs_minus(i, k) <= nbBandes) {
                    t_matrix y = matL(i, k) * matL(i, k) * matD.diagonal()[k] - compensation;
                    t_matrix t = sumDiag + y;
                    compensation = (t - sumDiag) - y;
                    sumDiag = t;
                }
            }

            matD.diagonal()[i] = matrix(i, i) - sumDiag;
            //matD[i] = matrix[i][i] - sumDiag; // doit être non-nul;




#ifdef DEBUG
            if (matD.diagonal()[i] >= 1.0E20) {
                qWarning() << "[Function::decompositionCholesky]  matD[i] ="<< static_cast<double>(matD.diagonal()[i]) << " >= 1.E+20 ";
            }
            if (matD.diagonal()[i] < 0) {
                std::cout << "[Function::decompositionCholesky] The matrix is not positive definite." << static_cast<double>(matD.diagonal()[i]) << std::endl;
                throw std::runtime_error("[Function::decompositionCholesky] The matrix is not positive definite.");
                //matD[i] = 0;
            }
#endif

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



    } catch(const char* e) {
        qCritical() << "[Function::decompositionCholesky] " << e;

    } catch(...) {
        qCritical() << "[Function::decompositionCholesky]  Caught Exception!\n";

    }


    return {matL, matD};
}
// https://hatefdastour.github.io/notes/Numerical_Analysis/chapter_07/LDL_decomposition.html
/**
 * @brief modifiedCholeskyDecomposition Décomposition de Cholesky alternative ou décomposition de Crout
 * @param matrix
 * @param epsilon
 * @return
 * https://en.wikipedia.org/wiki/Cholesky_decomposition
 */
std::pair<Matrix2D, DiagonalMatrixLD> modifiedCholeskyDecomposition(const Matrix2D& matrix, double epsilon = 1e-10)
{
    const size_t n = matrix.rows();
    Matrix2D L = Matrix2D::Zero (n, n);
    DiagonalMatrixLD D (n);

    for (size_t i = 0; i < n; ++i)
    {
        double sum = 0.0;
        for (size_t j = 0; j < i; ++j) {
            sum += L(i, j) * L(i, j) * D.diagonal()[j];
        }
        D.diagonal()[i] = matrix(i, i) - sum;

        // Modification pour rendre la matrice définie positive
        if (D.diagonal()[i] <= epsilon) {
            D.diagonal()[i] = epsilon;
        }

        L(i, i) = 1.0;
        for (size_t j = i + 1; j < n; ++j) {
            double sum2 = 0.0;
            for (size_t k = 0; k < i; ++k)
            {
                sum2 += L(j, k) * L(j, k) * D.diagonal()[k];
            }
            L(j, i) = (matrix(j, i) - sum2) / D.diagonal()[i];
        }
    }

    return std::make_pair(L, D);
}


/**
 * @brief MoreSorensenCholesky
 * Moré-Sorensen (MS): Conçu spécifiquement pour gérer les matrices non définies positives en les modifiant pour obtenir une approximation définie positive
 * Calcul la décomposition en LDL^t
 * @param A
 * @param regularization
 * @return
 * @ref https://mcsweeney90.github.io/files/modified-cholesky-decomposition-and-applications.pdf
 */
std::pair<Matrix2D, DiagonalMatrixLD> cholesky_LDLt_MoreSorensen(const Matrix2D& A, t_matrix regularization)
{
    size_t n = A.rows();
    t_matrix delta = regularization;

#ifdef DEBUG
    // Vérifier que la matrice est carrée
    for (size_t i = 0; i < n; i++) {
        if (size_t(A.cols()) != n) {
            throw std::invalid_argument("[cholesky_LDLt_MoreSorensen] Input matrix must be square");
        }
    }
#endif
    // Initialiser L comme une matrice identité
    Matrix2D L (n, n);
    for (size_t i = 0; i < n; i++) {
        L(i, i) = 1.0;
    }

    // Initialiser D
    DiagonalMatrixLD D (n);

    // Algorithme SE99 pour la décomposition LDL^T avec amélioration MS
    for (size_t j = 0; j < n; j++) {
        // Calculer D[j]
        D.diagonal()[j] = A(j, j);
        for (size_t k = 0; k < j; k++) {
            D.diagonal()[j] -= L(j, k) * L(j, k) * D.diagonal()[k];
        }

        // Appliquer la modification de Moré-Sorensen
        // Si D[j] est trop petit (matrice non définie positive)
        if (D.diagonal()[j] <= delta) {
            D.diagonal()[j] = std::max(delta, 0.01 * std::abs(D.diagonal()[j]));
        }

        // Calculer les éléments de L sous la diagonale
        for (size_t i = j + 1; i < n; i++) {
            L(i,j) = A(i, j);
            for (size_t k = 0; k < j; k++) {
                L(i,j ) -= L(i, k) * L(i, k) * D.diagonal()[k];
            }

            L(i, j) /= D.diagonal()[j];
        }
    }
    return std::make_pair(L, D);
}

// Constructeur pour la décomposition LDLt avec amélioration Moré-Sorensen d'une matrice de bande symétrique
std::pair<Matrix2D, DiagonalMatrixLD> banded_Cholesky_LDLt_MoreSorensen(const Matrix2D &A, int bandwidth, t_matrix regularization)
{
    size_t n = A.rows();
    size_t k = bandwidth;
    t_matrix delta = regularization;

#ifdef DEBUG
    // Vérifier que la matrice est carrée
    for (size_t i = 0; i < n; i++) {
        if (A.cols() != A.rows()) {
            throw std::invalid_argument("[banded_Cholesky_LLt_MoreSorensen] Input matrix must be square");
        }
    }
#endif

    Matrix2D L = Matrix2D::Zero (n, n);
    for (size_t i = 0; i < n; i++) {
        L(i, i) = 1.0;
    }

    DiagonalMatrixLD D (n);

    // Algorithme MS adapté pour matrices de bande
    for (size_t j = 0; j < n; j++) {
        // Calculer D[j]
        D.diagonal()[j] = A(j, j);

        // Limiter les calculs à la bande
        size_t start_row = std::max(size_t(0), j - k);
        for (size_t s = start_row; s < j; s++) {
            // Utiliser uniquement les éléments dans la bande
            if (j - s <= k) {
                D.diagonal()[j] -= L(j, s) * L(j, s) * D.diagonal()[s];
            }
        }

        // Appliquer la modification de Moré-Sorensen
        if (D.diagonal()[j] <= delta) {
            D.diagonal()[j] = std::max(delta, 0.01 * std::abs(D.diagonal()[j]));
        }

        // Calculer les éléments de L sous la diagonale dans la bande

        size_t end_row = std::min(n, j + k + 1);
        for (size_t i = j + 1; i < end_row; i++) {
            // Vérifier que l'élément est dans la bande
            if (i - j <= k) {
                L(i, j) = A(i, j);

                // Limiter les calculs aux éléments dans la bande
                int start_col = std::max(0, static_cast<int>(std::max(i - k, j - k)));

                for (size_t s = start_col; s < j; s++) {
                    // Utiliser uniquement les éléments dans la bande
                    if (i - s <= k && j - s <= k) {
                        L(i, j) -= L(i, s) * L(j, s) * D.diagonal()[s];
                    }
                }

                L(i, j) /= D.diagonal()[j];
            }
        }
    }
    return std::make_pair(L, D);
}


std::pair<Matrix2D, DiagonalMatrixLD> decompositionCholeskyKK(const Matrix2D &matrix, const size_t nbBandes, const size_t shift)
{
    //return BandedMoreSorensenCholesky(matrix, nbBandes, 0.0);

    errno = 0;
    //if (math_errhandling & MATH_ERREXCEPT) feclearexcept(FE_ALL_EXCEPT);


    const size_t dim = matrix.rows();
    Matrix2D matL = Matrix2D::Zero  (dim, dim);
    DiagonalMatrixLD matD (dim);

    if (dim - 2*shift == 1) { // cas des splines avec 3 points
        matD.diagonal()[1] = matrix(1, 1);
        matL(1, 1) = 1.;

    } else {

        // const int bande = floor((nbBandes-1)/2);

        // shift : décalage qui permet d'éliminer les premières et dernières lignes et colonnes constituées de zéro
        for (size_t i = shift; i < dim-shift; ++i) {
            matL(i, i) = 1.;
        }
        matD.diagonal()[shift] = matrix(shift, shift);

        try {
            for (size_t i = shift+1; i < dim-shift; ++i) {
                matL(i, shift) = matrix(i, shift) / matD.diagonal()[shift];
                /*   avec bande */
                for (size_t j = shift+1; j < i; ++j) {
                    if (abs_minus(i, j) <= nbBandes) {
                        t_matrix sum = 0.0;
                        /* for (size_t k = shift; k < j; ++k) {
                            if (abs_minus(i, k) <= nbBandes) {
                                sum += matL[i][k] * matD.at(k) * matL[j][k];
                            }
                        } */

                        t_matrix compensation = 0.0;  // terme de compensation
                        for (size_t k = shift; k < j; ++k) {
                            if (abs_minus(i, k) <= nbBandes){
                                t_matrix product = matL(i, k) * matD.diagonal()[k] * matL(j, k);
                                // Algorithme Kahan pour la sommation
                                t_matrix y = product - compensation;
                                t_matrix t = sum + y;
                                compensation = (t - sum) - y;
                                sum = t;
                            }
                        }


                        matL(i, j) = (matrix(i, j) - sum) / matD.diagonal()[j];
                    }
                }

                t_matrix sumDiag = 0.0;
                for (size_t k = shift; k < i; ++k) {
                    if (abs_minus(i, k) <= nbBandes) {
                        sumDiag += matL(i, k) * matL(i, k) * matD.diagonal()[k];
                    }
                }

                matD.diagonal()[i] = matrix(i, i) - sumDiag;


            }
//auto corrected_matrix = MoreSorensenCholesky(matrix);
//auto corrected_matrix1 = BandedMoreSorensenCholesky(matrix, nbBandes, 0.0);

            for (long i = 0 ; i < matD.rows(); i++) {
                auto d = matD.diagonal()[i];
                if (d <= 0) {
                    std::cout << "[Function::decompositionCholeskyKK] The matrix is not positive definite. " << static_cast<double>(d) << std::endl;
                    // throw std::runtime_error("[Function::decompositionCholeskyKK] The matrix is not positive definite.");

                    //return std::make_pair(Matrix2D(), MatrixDiag());
                    //auto corrected_matrix = modifiedCholeskyDecomposition(matrix);
                    auto corrected_matrix1 = banded_Cholesky_LDLt_MoreSorensen(matrix, nbBandes, 0.0);
                    //auto corrected_matrix = MoreSorensenCholesky(matrix);

                    return corrected_matrix1;
                    d = 0;
                }
            }

            // matL : Par exemple pour n = 5 et shift = 0:
            // 1 0 0 0 0
            // X 1 0 0 0
            // X X 1 0 0
            // X X X 1 0
            // X X X X 1

            // matL : Par exemple pour n = 5 et shift = 1:
            // 0 0 0 0 0
            // 0 1 0 0 0
            // 0 X 1 0 0
            // 0 X X 1 0
            // 0 0 0 0 0

        } catch(const char* e) {
            std::cout << "[Function::decompositionCholesky] " << e << std::endl;
            return std::make_pair(Matrix2D(), DiagonalMatrixLD());

        } catch(...) {
            std::cout << "[Function::decompositionCholeskyKK] : Caught Exception!" << std::endl;
            return std::make_pair(Matrix2D(), DiagonalMatrixLD());
        }
    }

    return std::pair<Matrix2D, DiagonalMatrixLD>(matL, matD);
}




/**
 * @brief resolutionSystemeLineaireCholesky, calcul vecGamma
 * @param decomp
 * @param vecQtY
 * @return
 */
std::vector<t_matrix> resolutionSystemeLineaireCholesky(const std::pair<Matrix2D, DiagonalMatrixLD>& decomp, const std::vector<t_matrix>& vecQtY)
{
    const Matrix2D& L = decomp.first;
    const DiagonalMatrixLD& D = decomp.second;
    const size_t n = D.rows();
    std::vector<t_matrix> vecGamma (n);
    std::vector<t_matrix> vecU (n);
    std::vector<t_matrix> vecNu (n);

    if (n > 3 ) {
        vecU[1] = vecQtY[1];
        vecU[2] = vecQtY[2] - L(2, 1) * vecU[1];

        for (size_t i = 3; i < n-1; ++i) {
            vecU[i] = vecQtY[i] - L(i, i-1) * vecU[i-1] - L(i, i-2) * vecU[i-2]; // pHd : Attention utilisation des variables déjà modifiées
        }

        for (size_t i = 1; i < n-1; ++i) {
            vecNu[i] = vecU[i] / D.diagonal()[i];
        }

        vecGamma[n-2] = vecNu.at(n-2);
        if (std::isnan(vecGamma[n-2]))
            vecGamma[n-2] = 0.0;

        vecGamma[n-3] = vecNu[n-3] - L(n-2, n-3) * vecGamma[n-2];
        if (std::isnan(vecGamma[n-3]))
            vecGamma[n-3] = 0.0;

        for (size_t i = n-4; i > 0; --i) {
            vecGamma[i] = vecNu[i] - L(i+1, i) * vecGamma[i+1] - L(i+2, i) * vecGamma[i+2]; // pHd : Attention utilisation des variables déjà modifiées
        }

    } else {
        // cas n = 3
        vecGamma[1] = vecQtY[1] / D.diagonal()[1];
    }

    return vecGamma;
}




/**
 * @brief decompositionLU0
 * example of good code https://www.codewithc.com/c-program-for-lu-factorization/
 * @param A
 * @return
 */
std::pair<Matrix2D, Matrix2D > decompositionLU0(const Matrix2D& A)
{
   const size_t n = A.rows();

    Matrix2D L = Matrix2D::Zero (n, n);
    Matrix2D U = Matrix2D::Zero (n, n);

    size_t i, j , k;

    for (i = 0; i < n; i++) {
        // Upper triangle
        for (k  = i; k < n; k++) {
            U(i ,k) = A(i, k);
            for (j = 0; j < i; j++)
                U(i, k) -=  U(j, k) * L(i, j);
        }

        // diagonal creation
        L(i, i) = 1.;

        // Lower triangle
        for (k = i+1; k < n; k++) {
            L(k, i) = A(k, i);
            for (j = 0; j < i; j++)
                L(k, i) -=  U(j, i) * L(k, j);

            L(k, i) /= U(i, i);
        }


    }

    return std::pair<Matrix2D, Matrix2D>(L, U);
}

std::pair<Matrix2D, Matrix2D> Doolittle_LU(const Matrix2D A)
{
    const size_t n =  A.rows();
    Matrix2D L = Matrix2D::Zero (n, n);
    Matrix2D U = Matrix2D::Zero (n, n);
    // Decomposing matrix into Upper and Lower
    // triangular matrix
    for (size_t i = 0; i < n; i++) {
        // Upper Triangular
        for (size_t k = i; k < n; k++) {
            // Summation of L(i, j) * U(j, k)
            t_matrix sum = 0;
            for (size_t j = 0; j < i; j++)
                sum += L(i, j) * U(j, k);

            // Evaluating U(i, k)
            U(i, k) = A(i, k) - sum;
        }

        // Lower Triangular
        for (size_t k = i; k < n; k++) {
            if (i == k)
                L(i, i) = 1; // Diagonal as 1
            else {
                // Summation of L(k, j) * U(j, i)
                t_matrix sum = 0;
                for (size_t j = 0; j < i; j++)
                    sum += L(k, j) * U(j, i);

                // Evaluating L(k, i)
                L(k, i) = (A(k, i) - sum) / U(i, i);

            }

        }

    }

    return std::pair<Matrix2D, Matrix2D>(L, U);
}

std::pair<Matrix2D, DiagonalMatrixLD> LU_to_LD(const std::pair<Matrix2D, Matrix2D> LU)
{
    DiagonalMatrixLD D (LU.second.rows());
    for (long i = 0; i< LU.second.rows(); i++) {
        D.diagonal()[i] = LU.second(i, i);
    }
    return std::pair<Matrix2D, DiagonalMatrixLD>(LU.first, D);
}

//https://en.wikipedia.org/wiki/QR_decomposition#Example
// faster than que householderQR
std::pair<Matrix2D, Matrix2D > decompositionQR(const Matrix2D& A)
{
   const long n = A.rows(); // on considère une matrice carré


   Matrix2D Mat_H (n, n);

   for (long i=0; i<n; i++)
       Mat_H(i, i) = 1.0;

   std::vector<t_matrix> Vec_V (n);


     // si matrice Mat carrée avec diml=dimc, faire k=1 to dimc-1
     // si matrice rectangulaire avec dimc<diml, faire k=1 to dimc
  /*   if diml=dimc then
        dim_fin:=dimc-1
     else
        dim_fin:=dimc;
  */
   auto Mat = A;
   for (long k = 0 ; k < n-1; k++) {

       t_matrix a2 = 0.;
       for (long i = k; i<n; i++)
           a2 += pow(A(i, k), 2.);

       t_matrix alpha = sqrt(a2);
       t_matrix beta = pow(alpha, 2) - alpha * A(k, k);

       // ajout Ph. Lanos en avril 2010 pour traiter le cas o˘ Mat est diag. partielle
   /*    if (beta == 0.) {
           Mat_QR_Q_res[k][k] = 1;
           Mat_QR_R_res[k][k] = A[k][k];
           break;
       }
  */
       //construction du vecteur V
       Vec_V[k] = A(k, k) - alpha;
       for (long i = k+1; i<n; i++)  Vec_V[i] = A(i, k); // diff extrac_column(A, k) because start with i=k+1

       //construction de Ak+1

       for (long j = k; j < n; j++) {
           t_matrix som = 0;
           for (long i = k; i < n; i++)
               som += Vec_V[i] * Mat(i, j);

           t_matrix c = som/beta;

           for (long i = k; i < n; i++)
               Mat(i, j) -= c*Vec_V[i];
       }

       //construction de H  = Hk ....H2 H1
       for (long j = 0; j < n; j++) {
           double som = 0.;
           for (long i = k; i < n; i++)
               som += Vec_V[i] * Mat_H(i, j);

           t_matrix c = som/beta;

           for (long i = k; i < n; i++)
               Mat_H(i, j) -= c*Vec_V[i];
       }


   }  //fin boucle sur k

   // Q est la transposée de H
  // Matrix2D Q = transpose0(Mat_H);

   // R est Ègal à Mat
  // Matrix2D& R = Mat;

    return std::pair<Matrix2D, Matrix2D>(transpose0(Mat_H), Mat);
}

// Householder algo

//   ||x||
t_matrix norm (const std::vector<t_matrix>& a)
{
    double sum = 0;
    for (size_t i = 0; i < a.size(); i++)
        sum += a[i] * a[i];
    return sqrt(sum);
}

void rescale(std::vector<t_matrix>& a, t_matrix factor)
{
    for (size_t i = 0; i < a.size(); i++)
        a[i] /= factor;
}

void rescale_unit(std::vector<t_matrix>& a)
{
    t_matrix factor = norm(a);
    rescale(a, factor);
}

// c = a + b * s
void vmadd(const std::vector<t_matrix>& a, const std::vector<t_matrix>& b, double s, std::vector<t_matrix>& c)
{
  if (c.size() != a.size() or c.size() != b.size()) {
      std::cout<<a.size() << b.size()<< c.size();
      std::cerr << "[vmadd]: vector sizes don't match\n";
    return;
  }

  for (size_t i = 0; i < c.size(); i++)
    c[i] = a[i] + s * b[i];
}

// mat = I - 2*v*v^T
// !!! m is allocated here !!!
void compute_householder_factor(Matrix2D& A, std::vector<t_matrix>& v)
{
  size_t n = v.size();
  A (n, n);
  for (size_t i = 0; i < n; i++) {

      auto v_i = -2. * v[i];
      for (size_t j = 0; j < n; j++)
          A(i, j) =  v_i * v[j];
  }

  for (size_t i = 0; i < n; i++)
      A(i, i) += 1.0;
}

// take c-th column of a matrix, put results in Vector v
std::vector<t_matrix> extract_column(Matrix2D& A, const int c)
{
  std::vector<t_matrix> v (A.rows());
  for (long i = 0; i < A.rows(); i++)
        v[i] = A(i, c);

    return v;
}

// compute minor ??
Matrix2D compute_minor(const Matrix2D& A, const long d)
{
    const long n = A.rows();
    Matrix2D M (n, n);

    for (long i = 0; i < d; i++)
      M(i, i) = 1.0;

    for (long i = d; i < n; i++) {
        for (long j = d; j < n; j++)
            M(i, j) = A(i, j);
    }

    return M;
}

/**
 * @brief householder compute decomposition QR of a matrix A
 * @ref Golub, G. H., Van Loan, C. F., 2013. Matrix computations. Johns Hopkins studies in the mathematical sciences. The Johns Hopkins University Press, Baltimore.
 * @ref https://en.wikipedia.org/wiki/QR_algorithm#The_implicit_QR_algorithm
 * @param A
 *
 * @return pair of Matrix2D Q and R
 */

std::pair<Matrix2D, Matrix2D> householderQR(Matrix2D& A)
{

  size_t m = A.rows();
  size_t n = A.rows();

  // array of factor Q1, Q2, ... Qm
  std::vector<Matrix2D> qv(m);

  // temp array
  Matrix2D z = A;
  Matrix2D z1 = Matrix2D::Zero (n, m);

  for (size_t k = 0; k < n && k < m-1 ; k++) {

    std::vector<t_matrix> e(m), x(m);
    t_matrix a;

    // compute minor
    z1 = compute_minor(z, k);

    // extract k-th column into x
    x = extract_column(z1, k);

    a = norm(x);
    if (A(k, k) > 0) a = -a;

    for (size_t i = 0; i < e.size(); i++)
      e[i] = (i == k) ? 1 : 0;

    // e = x + a*e
    vmadd(x, e, a, e);

    // e = e / ||e||
    rescale_unit(e);

    // qv[k] = I - 2 *e*e^T
    compute_householder_factor(qv[k], e);

    // z = qv[k] * z1
   // z = multiplyWinograd0(qv[k], z1);
    z = multiplyMatrix_Naive(qv[k], z1);

  }

  Matrix2D Q = qv[0];

  // after this loop, we will obtain Q (up to a transpose operation)
  for (size_t i = 1; i < n && i < m-1 ; i++) {

    //z1 = multiplyWinograd0(qv[i], Q);
      z1 = multiplyMatrix_Naive(qv[i], Q);
    Q = std::move(z1);

  }

  //R = multiplyWinograd0(Q, mat);
  Matrix2D R = multiplyMatrix_Naive(Q, A);
  Q = transpose0(Q);

 return std::pair<Matrix2D, Matrix2D>(Q, R);
}

/**
 * @brief solve_quadratic y = ax^2 + bx + c
 * @param a
 * @param b
 * @param c
 * @return y1, y2
 */
std::pair<double, double> solve_quadratic(const double y, const double a, const double b, const double c)
{
    double y1, y2;

    if (a == 0.) { // linear equation
        if (b == 0.) { //constant line
            y1 = INFINITY;
            y2 = INFINITY;

        } else { // linear
            y1 = (y - c) / b;
            y2 = y1;
        }

    } else {
        const double delta = std::pow(b, 2.) - 4*a*(c - y);
        if (delta > 0) {

            const double s1 = (- b - sqrt(delta)) / (2 * a);
            const double s2 = (- b + sqrt(delta)) / (2 * a);
            if (s1<s2) {
                y1 = s1;
                y2 = s2;

            } else {
                y1 = s2;
                y2 = s1;
            }

        } else {
            y1 = 0.;
            y2 = 0.;
        }

    }

    return std::pair<double, double>{y1, y2};
}


/**
 * @brief gaussian_filter, we assume a uniform step between values.
 * @param map
 * @param sigma, of the gaussian
 * @return
 */
std::vector<double> gaussian_filter(std::vector<double>& curve_input, const double sigma, const short padding_type)
{

    //qDebug() <<"[gaussian_filter]";
    //  data
    const int inputSize = curve_input.size();

    //const double step = 1.0 / static_cast<double>(inputSize - 1);

    /* ----- FFT -----
        http://www.fftw.org/fftw3_doc/One_002dDimensional-DFTs-of-Real-Data.html#One_002dDimensional-DFTs-of-Real-Data
        https://jperalta.wordpress.com/2006/12/12/using-fftw3/
    */


    const double sigma_filter = sigma;// * step;

    const int gaussSize = std::max(inputSize, int(3*sigma));
    const int paddingSize = 2*gaussSize;

    const int N = gaussSize + 2*paddingSize;
    //const int NComplex = 2* (N/2)+1;

    const int NComplex =  (N/2)+1;

    // https://www.fftw.org/fftw3_doc/Real_002ddata-DFT-Array-Format.html

    double *inputReal;
    inputReal = new double [N];


    fftw_complex *inputComplex;
    inputComplex = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * NComplex);

    if (padding_type == 0) {
        for (int i  = 0; i< paddingSize; i++) {
            inputReal[i] = 0;
        }
        for (int i = inputSize+paddingSize; i< N; i++) {
            inputReal[i] = 0;
        }

    } else if (padding_type == 1) {
        for (int i  = 0; i< paddingSize; i++) {
            inputReal[i] = curve_input[0];
        }
        for (int i = inputSize+paddingSize; i< N; i++) {
            inputReal[i] = curve_input[inputSize-1];
        }
    }
    // we could use std::copy

    for (int i = 0; i< inputSize; i++) {
        inputReal[i+paddingSize] = curve_input[i];
    }

    fftw_plan plan_input = fftw_plan_dft_r2c_1d(N, inputReal, inputComplex, FFTW_ESTIMATE);

    fftw_execute(plan_input);

    for (int i = 0; i < NComplex; ++i) {
        const double s =  M_PI * (double)i / (double)N;
        const double factor = exp(-2. * pow(s * sigma_filter, 2.));
        if (isnan(factor)) {
            qDebug()<<"gaussian filter"<< s << " isnan";
        }
        inputComplex[i][0] *= factor;
        inputComplex[i][1] *= factor;

    }


    double *outputReal;
    outputReal = new double [N];// [2* (N/2)+1];

    fftw_plan plan_output = fftw_plan_dft_c2r_1d(N, inputComplex, outputReal, FFTW_ESTIMATE);
    fftw_execute(plan_output);

    std::vector<double> results;
    for ( int i = 0; i < inputSize; i++) {
        results.push_back(outputReal[i + paddingSize]/N);
    }

    fftw_destroy_plan(plan_input);
    fftw_destroy_plan(plan_output);
    fftw_free(inputComplex);

    delete [] inputReal;
    delete [] outputReal;

    fftw_cleanup();
    return results;
}


std::vector<long double> gaussian_filter(std::vector<long double>& curve_input, const double sigma, const short padding_type)
{

    std::vector<double> c_input (curve_input.begin(), curve_input.end());
    const auto& c_output = gaussian_filter(c_input, sigma, padding_type);
    return std::vector<long double> (c_output.begin(), c_output.end());



    //qDebug() <<"[gaussian_filter]";
    //  data
 /*   const int inputSize = curve_input.size();

    //const double step = 1.0 / static_cast<double>(inputSize - 1);


    const long double sigma_filter = sigma;

    const int gaussSize = std::max(inputSize, int(3*sigma));
    const int paddingSize = 2*gaussSize;

    const int N = gaussSize + 2*paddingSize;
    //const int NComplex = 2* (N/2)+1;

    const int NComplex =  (N/2)+1;







    // https://www.fftw.org/fftw3_doc/Real_002ddata-DFT-Array-Format.html


    long double *inputReal;
    inputReal = new long double [N];


    fftwl_complex *inputComplex;
    inputComplex = (fftwl_complex*) fftwl_malloc(sizeof(fftwl_complex) * NComplex);

    if (padding_type == 0) {
        for (int i  = 0; i< paddingSize; i++) {
            inputReal[i] = 0;
        }
        for (int i = inputSize+paddingSize; i< N; i++) {
            inputReal[i] = 0;
        }

    } else if (padding_type == 1) {
        for (int i  = 0; i< paddingSize; i++) {
            inputReal[i] = curve_input[0];
        }
        for (int i = inputSize+paddingSize; i< N; i++) {
            inputReal[i] = curve_input[inputSize-1];
        }
    }
    // we could use std::copy

    for (int i = 0; i< inputSize; i++) {
        inputReal[i+paddingSize] = curve_input[i];
    }

    fftwl_plan plan_input = fftwl_plan_dft_r2c_1d(N, inputReal, inputComplex, FFTW_ESTIMATE);

    fftwl_execute(plan_input);

    for (int i = 0; i < NComplex; ++i) {
        const long double s = M_PI * (long double)i / (long double)N;
        const long double factor = expl(-2.0L * powl(s * sigma_filter, 2.0L));
        if (std::isnan(factor)) {
            qDebug() << "gaussian filter" << (double)s << " isnan";
        }
        inputComplex[i][0] *= factor;
        inputComplex[i][1] *= factor;
    }


    long double *outputReal;
    outputReal = new long double [N];

    fftwl_plan plan_output = fftwl_plan_dft_c2r_1d(N, inputComplex, outputReal, FFTW_ESTIMATE);
    fftwl_execute(plan_output);

    std::vector<long double> results;
    for ( int i = 0; i < inputSize; i++) {
        results.push_back(outputReal[i + paddingSize]/N);
    }

    fftwl_destroy_plan(plan_input);
    fftwl_destroy_plan(plan_output);
    fftwl_free(inputComplex);

    delete [] inputReal;
    delete [] outputReal;

    fftwl_cleanup();
    return results;
    */
}


/**
 * @brief gaussian_filter, we assume a uniform step between values.
 * @param map
 * @param sigma, of the gaussian
 * @return
 */
QMap<double, double> gaussian_filter(QMap<double, double> &map, const double sigma)
{
    std::vector<double> curve_input;

    const double min = map.firstKey();
    const double max = map.lastKey();

    const double step = (max - min) / (map.size()-1);

    for (auto [key, value] : map.asKeyValueRange()) {
        curve_input.push_back(value);
    }

    /* ----- FFT -----
        http://www.fftw.org/fftw3_doc/One_002dDimensional-DFTs-of-Real-Data.html#One_002dDimensional-DFTs-of-Real-Data
        https://jperalta.wordpress.com/2006/12/12/using-fftw3/
    */



    //qDebug() <<"filtre Gaussian";
    //  data
    const int inputSize = (int) curve_input.size();

    const double sigma_filter = sigma * step;

    const int gaussSize = std::max(inputSize, int(3*sigma));
    const int paddingSize = 2*gaussSize;

    const int N = gaussSize + 2*paddingSize;
    //const int NComplex = 2* (N/2)+1;

    const int NComplex =  (N/2)+1;

    // https://www.fftw.org/fftw3_doc/Real_002ddata-DFT-Array-Format.html

    double *inputReal;
    inputReal = new double [N];


    fftw_complex *inputComplex;
    inputComplex = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * NComplex);


    // we could use std::copy
    for (int i  = 0; i< paddingSize; i++) {
        inputReal[i] = curve_input[0];//0.;
    }
    for (int i = 0; i< inputSize; i++) {
        inputReal[i+paddingSize] = curve_input[i];
    }
    for (int i ( inputSize+paddingSize); i< N; i++) {
        inputReal[i] = curve_input[inputSize-1];//0.;
    }
    fftw_plan plan_input = fftw_plan_dft_r2c_1d(N, inputReal, inputComplex, FFTW_ESTIMATE);

    fftw_execute(plan_input);

    for (int i = 0; i < NComplex; ++i) {
        const double s =  M_PI * (double)i / (double)NComplex;
        const double factor = exp(-2. * pow(s * sigma_filter, 2.));
        if (isnan(factor)) {
            qDebug()<<"gaussian filter"<< s << " isnan";
        }
        inputComplex[i][0] *= factor;
        inputComplex[i][1] *= factor;

    }


    double *outputReal;
    outputReal = new double [2* (N/2)+1];//;[N];


    fftw_plan plan_output = fftw_plan_dft_c2r_1d(N, inputComplex, outputReal, FFTW_ESTIMATE);
    fftw_execute(plan_output);

    QMap<double, double> results;
    for ( int i = 0; i < inputSize; i++) {
        const double t = min + i* step;
        results[t] = outputReal[i + paddingSize]/N;
#ifdef DEBUG
        if (isnan(results[t])) {
            qDebug()<<"gaussian filter"<<t<< " isnan";
        }
#endif
    }

    fftw_destroy_plan(plan_input);
    fftw_destroy_plan(plan_output);
    fftw_free(inputComplex);

    delete [] inputReal;
    delete [] outputReal;

    fftw_cleanup();
    return results;
}

/**
 * @brief low_pass_filter, Since the signal exhibits a discontinuity,
 * the filtered signal exhibits a significant disturbance at the extremities.
 * (see Hanning filter)
 * @param map
 * @param Tc, Cut-off period, equal to 1/(cut-off frequency)
 * @param padding_type, 0 for zero value in padding, 1 for extremum value in padding
 * @return
 */

std::vector<double> low_pass_filter(std::vector<double>& curve_input, const double Tc, const short padding_type)
{
    const int inputSize = (int)curve_input.size();
    const int paddingSize = 2*inputSize;

    const int N = inputSize + 2*paddingSize;

    const int NComplex = 2* (N/2)+1;

    double *inputReal;
    inputReal = new double [N];

    fftw_complex *inputComplex;
    inputComplex = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * NComplex);

    if (padding_type == 0) {
        for (int i  = 0; i< paddingSize; i++) {
            inputReal[i] = 0;
        }
        for (int i = inputSize+paddingSize; i< N; i++) {
            inputReal[i] = 0;
        }

    } else if (padding_type == 1) {
        for (int i  = 0; i< paddingSize; i++) {
            inputReal[i] = curve_input.front();
        }
        for (int i = inputSize+paddingSize; i< N; i++) {
            inputReal[i] = curve_input.front();
        }
    }

    auto begin_map = curve_input.begin();
    for (int i = 0; i< inputSize; i++) {
        inputReal[i+paddingSize] = *begin_map++;
    }


    fftw_plan plan_input = fftw_plan_dft_r2c_1d(N, inputReal, inputComplex, FFTW_ESTIMATE);
    fftw_execute(plan_input);

    //

    // Ici, Fs est défini comme 1 (par point)
    double Fs = 1.0;
    double delta_f = Fs / static_cast<double>(N);  // delta_f = 1/N
    // Conversion de Tc en fréquence de coupure : f_c = 1 / (2π * Tc)
    double f_c = 1.0 / (2.0 * M_PI * Tc);
    // Indice de coupure dans le domaine fréquentiel
    int n_cut = static_cast<int>(f_c / delta_f);  // n_cut = N/(2π * Tc)

    // Pour éviter de dépasser le tableau FFT
    if(n_cut >= NComplex) {
        n_cut = NComplex - 1;
    }

    // Filtrage passe-bas : on conserve les basses fréquences pour i <= n_cut
    for (int i = 0; i < NComplex; ++i) {
        if (i > n_cut) {
            inputComplex[i][0] = 0;
            inputComplex[i][1] = 0;
        }
    }

    double *outputReal;
    outputReal = new double [N];

    fftw_plan plan_output = fftw_plan_dft_c2r_1d(N, inputComplex, outputReal, FFTW_ESTIMATE);
    fftw_execute(plan_output);

    std::vector<double> results;
    results.reserve(inputSize);

    for(int i = 0; i < inputSize; i++) {
        results.push_back(outputReal[i + paddingSize]/N);
    }

    fftw_destroy_plan(plan_input);
    fftw_destroy_plan(plan_output);
    fftw_free(inputComplex);
    delete [] inputReal;
    delete [] outputReal;
    fftw_cleanup();

    return results;
}



/**
 * @brief Version simplifiée sans FFT pour les petites courbes
 *
 * Plus simple et souvent plus stable pour de petites courbes (< 100 points)
 * sigma = 0.5-1.0 : Lissage léger, préserve les détails
 * sigma = 2.0-5.0 : Lissage modéré, bon compromis
 * sigma > 5.0 : Lissage fort, courbe très lisse
 */
QMap<double, double> gaussian_filter_simple(const QMap<double, double> &map, const double sigma)
{
    if (map.isEmpty() || sigma <= 0) return map;

    QMap<double, double> result;
    const double step = (map.lastKey() - map.firstKey()) / (map.size() - 1);
    const double sigma_filter = sigma * step;
    const int kernel_size = static_cast<int>(6 * sigma) | 1; // Impair
    const int half_kernel = kernel_size / 2;

    // Pré-calculer le noyau gaussien
    std::vector<double> kernel(kernel_size);
    double kernel_sum = 0.0;
    for (int i = 0; i < kernel_size; ++i) {
        const double x = (i - half_kernel) * step;
        kernel[i] = std::exp(-0.5 * std::pow(x / sigma_filter, 2));
        kernel_sum += kernel[i];
    }

    // Normaliser le noyau
    for (double &k : kernel) {
        k /= kernel_sum;
    }

    // Appliquer le filtre
    auto keys = map.keys();
    auto values = map.values();

    for (int i = 0; i < values.size(); ++i) {
        double filtered_value = 0.0;
        double weight_sum = 0.0;

        for (int j = -half_kernel; j <= half_kernel; ++j) {
            int idx = i + j;
            if (idx >= 0 && idx < values.size()) {
                const double weight = kernel[j + half_kernel];
                filtered_value += values[idx] * weight;
                weight_sum += weight;
            }
        }

        result[keys[i]] = filtered_value / weight_sum;
    }

    return result;
}
