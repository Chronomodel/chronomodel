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

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "DateUtils.h"
#include "Matrix.h"

#include <QMap>
#include <QVector>
#include <cmath>

#if PARALLEL
#include <execution>
#define PAR std::execution::par,
#else
#define PAR
#endif

typedef double type_data;

struct Quartiles{
    type_data Q1 = (type_data)0.;
    type_data Q2 = (type_data)0.;
    type_data Q3 = (type_data)0.;
};

struct FunctionStat{
    type_data max = (type_data)0.;
    type_data mode = (type_data)0.;
    type_data mean = (type_data)0.;
    type_data std = (type_data)0.;
    Quartiles quartiles;
};

struct TraceStat{
    type_data min = (type_data)0.;
    type_data max = (type_data)0.;
    type_data mean = (type_data)0.;
    type_data std = (type_data)0.;
    Quartiles quartiles;
};



struct DensityAnalysis
{
    FunctionStat funcAnalysis;
    TraceStat traceAnalysis;
};

FunctionStat analyseFunction(const QMap<type_data, type_data> &fun);
FunctionStat analyseFunction(const std::map<type_data, type_data> &fun);

QString FunctionStatToString(const FunctionStat& analysis);
QString densityAnalysisToString(const DensityAnalysis& analysis);

// Standard Deviation of a vector of data

double variance_Knuth(const QList<double> &data);
double variance_Knuth(const std::vector<double> &data);
double variance_Knuth(const std::vector<t_matrix> &data);
double variance_Knuth(const std::vector<int> &data);

type_data std_Koening(const QList<type_data> &data);
inline double std_Knuth(const QList<double> &data) {return sqrt(variance_Knuth(data));};
inline double std_Knuth(const std::vector<double> &data) {return sqrt(variance_Knuth(data));};
inline double std_Knuth(const std::vector<int> &data) {return sqrt(variance_Knuth(data));};



void mean_variance_Knuth(const std::vector<double> &data, double &mean, double &variance);
void mean_variance_Knuth(const QList<double> &data, double &mean, double &variance);

double std_unbiais_Knuth(const QList<double> &data);
double std_unbiais_Knuth(const std::vector<double> &data);
double std_unbiais_Knuth(const std::vector<int> &data);
void mean_std_unbiais_Knuth(const std::vector<int> &data, double& mean, double& std);

double covariance(const std::vector<double>& dataX, const std::vector<double>& dataY);

QList<double> autocorrelation_schoolbook(const QList<double> &trace, const int hmax = 40);
std::vector<double> autocorrelation_schoolbook(const std::vector<double> &trace, const int hmax = 40);

QList<double> autocorrelation_by_convol(const QList<double> &trace, const int hmax=40);

const std::pair<double, double> linear_regression(const std::vector<double>& dataX, const std::vector<double>& dataY);

double shrinkageUniform(const double s02);

/**
 * @brief Évalue la densité de probabilité de la loi normale (fonction de Gauss).
 *
 * La fonction renvoie la valeur de :
 * \f[
 * f(x \,|\, \mu, \sigma) =
 * \frac{1}{\sigma \sqrt{2\pi}}
 * \exp\!\left( -\tfrac{1}{2} \left(\frac{x - \mu}{\sigma}\right)^2 \right)
 * \f]
 *
 * @param x      Valeur en laquelle la densité est évaluée.
 * @param mu     Moyenne de la loi normale (par défaut 0.0).
 * @param sigma  Écart-type de la loi normale (par défaut 1.0, doit être > 0).
 * @return       Valeur de la densité de probabilité \f$f(x|\mu,\sigma)\f$.
 *
 * @note Optimisé : constante \f$1/\sqrt{2\pi}\f$ précalculée, pas d'appel inutile à pow().
 * @warning Si @p sigma <= 0, le résultat n'est pas défini.
 */
inline double dnorm(const double x, const double mu = 0.0, const double sigma = 1.0)
{
    static constexpr double inv_sqrt_2pi = 0.3989422804014327; // 1 / sqrt(2π)
    double z = (x - mu) / sigma;
    return inv_sqrt_2pi / sigma * std::exp(-0.5 * z * z);
}

Quartiles quartilesForTrace(const QList<type_data> &trace);
Quartiles quartilesForTrace(const std::vector<type_data> &trace);

TraceStat traceStatistic(const QList<type_data> &trace);
TraceStat traceStatistic(const std::vector<type_data> &trace);

// QList<double> calculRepartition (const QList<double> &calib);
QList<double> calculRepartition (const QMap<double, double> &calib);
std::vector<double> calculRepartition(const std::map<double, double>  &calib);

Quartiles quartilesForRepartition(const QList<double> &repartition, const double tmin, const double step);
Quartiles quartilesForRepartition(const std::vector<double> &repartition, const double tmin, const double step);

std::pair<double, double> credibilityForTrace(const QList<double> &trace, double thresh, double &exactThresholdResult, const QString description = "Credibility computation");
std::pair<double, double> credibilityForTrace(const QList<int> &trace, double thresh, double &exactThresholdResult, const QString description = "Credibility computation");
std::pair<double, double> credibilityForTrace(const std::vector<double> &trace, double thresh, double& exactThresholdResult, const QString description = "Credibility computation");

std::pair<double, double> timeRangeFromTraces(const QList<double> &trace1, const QList<double> &trace2, const double thresh, const QString description ="Time Range Computation");
std::pair<double, double> timeRangeFromTraces(const std::vector<double> &trace1, const std::vector<double> &trace2, const double thresh, const QString description ="Time Range Computation");

std::pair<double, double> gapRangeFromTraces(const QList<double> &trace1, const QList<double> &trace2, const double thresh, const QString description ="Gap Range Computation");
std::pair<double, double> gapRangeFromTraces(const std::vector<double> &trace1, const std::vector<double> &trace2, const double thresh, const QString description ="Gap Range Computation");

std::pair<double, double> transitionRangeFromTraces(const QList<double> &trace1, const QList<double> &trace2, const double thresh, const QString description ="Gap Range Computation");
std::pair<double, double> transitionRangeFromTraces(const std::vector<double> &trace1, const std::vector<double> &trace2, const double thresh, const QString description ="Gap Range Computation");

const QString interval_to_text(const QPair<double, QPair<double, double> > &interval, DateConversion conversionFunc = nullptr, const bool forCSV = false);

const QString get_HPD_text_from_mapping(const std::map<double, double> &mapping, const QString &unit = QString(),  DateConversion conversionFunc = nullptr, const bool forCSV = false);
const QString get_HPD_text(const QList<QPair<double, QPair<double, double> >> &intervals, const QString &unit = QString(),  DateConversion conversionFunc = nullptr, const bool forCSV = false);

QList<QPair<double, QPair<double, double> > > intervals_hpd_from_mapping(const std::map<double, double> &area_mapping, double &real_thresh);
QList<QPair<double, QPair<double, double> > > intervals_hpd_from_mapping(const std::map<double, double> &area_mapping);

//-------- Matrix
std::vector<t_matrix> initVector(size_t n);

//std::vector<std::vector<int>> initIntMatrix(size_t rows, size_t cols);
//std::vector<std::vector<double>> initMatrix(size_t rows, size_t cols);
void resizeMatrix(std::vector<double> &matrix,  size_t rows, size_t cols);

std::vector<long double> initLongVector(size_t n);

t_matrix trace_kahan(const MatrixLD& mat);

t_matrix determinant(const MatrixLD& matrix, size_t shift = 0); // à contrôler
t_matrix determinant_gauss(const MatrixLD &matrix, size_t shift = 0);

MatrixLD seedMatrix(const MatrixLD& matrix, size_t shift = 0);
MatrixLD remove_bands_Matrix(const MatrixLD& matrix, size_t shift = 0);

MatrixLD transpose0(const MatrixLD& matrix);
MatrixLD transpose(const MatrixLD& matrix, const size_t nbDiag);

MatrixLD multiMatParDiag0(const MatrixLD& matrix, const DiagonalMatrixLD &diag);
MatrixLD multiMatParDiag(const MatrixLD& matrix, const DiagonalMatrixLD &diag, size_t nbBandes);

MatrixLD multiDiagParMat0(const DiagonalMatrixLD &diag, const MatrixLD& matrix);
MatrixLD multiDiagParMat(const DiagonalMatrixLD &diag, const MatrixLD& matrix, const size_t nbBandes);

t_matrix quadratic_form(const MatrixLD& A, const std::vector<t_matrix>& X);
t_matrix quadratic_form(const MatrixLD& K, const MatrixLD& Y);
double quadratic_form(const MatrixD& K, const MatrixD& Y);

std::vector<t_matrix> multiMatParVec(const MatrixLD& matrix, const std::vector<t_matrix>& vec, const size_t nbBandes);
std::vector<double> multiMatParVec(const MatrixD &matrix, const std::vector<double> &vec, const size_t nbBandes);


MatrixLD addMatEtMat0(const MatrixLD& matrix1, const MatrixLD& matrix2);
MatrixLD addMatEtMat(const MatrixLD& matrix1, const MatrixLD& matrix2, const size_t nbBandes);
MatrixLD addIdentityToMat(const MatrixLD& matrix);
MatrixLD multiConstParMat(const MatrixLD& matrix, const double c, const size_t nbBandes);
MatrixLD multiConstParMat0(const MatrixLD& matrix, const double c);

MatrixLD multiMatParMat0(const MatrixLD& matrix1, const MatrixLD& matrix2);
MatrixLD multiMatParMat(const MatrixLD& matrix1, const MatrixLD& matrix2, const size_t nbBandes1, const size_t nbBandes2);

MatrixLD addDiagToMat(const DiagonalMatrixLD& diag, MatrixLD matrix);

MatrixLD soustractMatToIdentity(const MatrixLD& matrix);

MatrixLD multiplyMatrix_Naive(const MatrixLD& a, const MatrixLD& b);
MatrixLD multiplyMatrix_Winograd(const MatrixLD& a, const MatrixLD& b);
MatrixLD multiplyMatrixBanded_Winograd(const MatrixLD& a, const MatrixLD& b,  const int bandwidth = 0);

MatrixLD inverseMatSym0(const MatrixLD& matrix, const size_t shift = 0);
MatrixLD inverse_padded_matrix(const MatrixLD& paddedMatrix, const long shift = 1);
t_matrix determinant_padded_matrix(const MatrixLD& paddedMatrix, const long shift = 1);
t_matrix ln_rate_determinant_padded_matrix_A_B(const MatrixLD& paddedMatrix_A,
                                               const MatrixLD& paddedMatrix_B,
                                               const long shift = 1);
double ln_rate_determinant_padded_matrix_A_B(const MatrixD& paddedMatrix_A,
                                               const MatrixD& paddedMatrix_B,
                                               const long shift = 1);

Eigen::MatrixXd geninv(const Eigen::MatrixXd& G);
MatrixLD geninv(const MatrixLD& G);
Eigen::MatrixXd pseudoInverseSVD(const Eigen::MatrixXd& G, double tol = 1e-9);
MatrixLD pseudoInverseSVD(const MatrixLD& G, t_matrix tol = 1e-20);
MatrixLD inverse_banded_padded_matrix(const MatrixLD& bandedPaddedMatrix, const long shift = 1);

MatrixLD inverseMatSym(const MatrixLD& matrix1, const DiagonalMatrixLD& matrix2, const size_t nbBandes, const size_t shift);

MatrixLD choleskyInvert(const std::pair<MatrixLD, DiagonalMatrixLD> &decomp);
MatrixLD inverseMatSym_origin(const std::pair<MatrixLD, DiagonalMatrixLD>& decomp, const size_t nbBandes, const size_t shift);
MatrixD inverseMatSym_origin(const std::pair<MatrixD, DiagonalMatrixD> &decomp, const size_t nbBandes, const size_t shift);


double sumAllMatrix(const std::vector<std::vector<double>>& matrix);
t_matrix sumAllMatrix(const MatrixLD& m);


double sumAllVector(const std::vector<double>& matrix);

MatrixLD cofactor0(const MatrixLD& matrix);
MatrixLD comatrice0(const MatrixLD& matrix);

#pragma mark Decomposition
MatrixLD choleskyLL0(const MatrixLD& matrix);
MatrixLD cholesky_LLt_MoreSorensen(const MatrixLD &matrix);
MatrixLD cholesky_LLt_MoreSorensen_adapt(const MatrixLD &matrix);
MatrixLD robust_LLt(const MatrixLD& matrix);
MatrixD robust_LLt(const MatrixD& matrix);

MatrixLD robust_SVD(const MatrixLD& V);
MatrixD robust_SVD(const MatrixD& V) ;


std::pair<MatrixLD, DiagonalMatrixLD > choleskyLDLT(const MatrixLD& matrix);
std::pair<MatrixLD, DiagonalMatrixLD > choleskyLDLT(const MatrixLD& matrix, const size_t shift);
std::pair<MatrixLD, DiagonalMatrixLD > choleskyLDLT(const MatrixLD& matrix, const size_t nbBandes, const size_t shift);
std::pair<MatrixLD, DiagonalMatrixLD > choleskyLDLT_Dsup0(const MatrixLD& matrix, const size_t nbBandes, const size_t shift);
std::pair<MatrixLD, DiagonalMatrixLD> decompositionCholesky(const MatrixLD& matrix, const size_t nbBandes, const size_t shift);
std::pair<MatrixD, DiagonalMatrixD> decompositionCholesky(const MatrixD &matrix, const size_t nbBandes, const size_t shift);

std::pair<MatrixLD, DiagonalMatrixLD> decompositionCholeskyKK(const MatrixLD& matrix, const size_t nbBandes, const size_t shift);
// algoithm with More and Sorensen adaptation
std::pair<MatrixLD, DiagonalMatrixLD> cholesky_LDLt_MoreSorensen(const MatrixLD& A, t_matrix regularization = 0.0);
std::pair<MatrixLD, DiagonalMatrixLD> banded_Cholesky_LDLt_MoreSorensen(const MatrixLD& A, int bandwidth, t_matrix regularization = 1e-10);

std::pair<MatrixLD, MatrixLD > decompositionLU0(const MatrixLD& A);
std::pair<MatrixLD, MatrixLD> Doolittle_LU(const MatrixLD A);
std::pair<MatrixLD, DiagonalMatrixLD> LU_to_LD(const std::pair<MatrixLD, MatrixLD> LU);

std::pair<MatrixLD, MatrixLD > decompositionQR(const MatrixLD& A);
std::pair<MatrixLD, MatrixLD> householderQR(MatrixLD& A);

std::vector<t_matrix> resolutionSystemeLineaireCholesky(const std::pair<MatrixLD, DiagonalMatrixLD> &decomp, const std::vector<t_matrix>& vecQtY);
std::vector<double> resolutionSystemeLineaireCholesky(const std::pair<MatrixD, DiagonalMatrixD>& decomp, const std::vector<double>& vecQtY);

inline double rounddouble(const double f, const int prec)
{
    double result;
    if (prec > 0){
        const double factor = pow(10., (double)prec);
        result = round(f * factor) / factor;
    } else {
        result = round(f);
    }
    return result;
}

template <typename T>
inline bool isOdd( T value )
{
    return (value % 2!= 0 ? true : false);
}

template <typename T>
inline bool isEven( T value )
{
    return (value % 2 == 0 ? true : false);
}


/**
 * @brief gammaQuartile used with quantile, find the gamma coef corresponding to the
 * type of R Calcul, to use with the general formula
 * @param trace
 * @param quartileType
 * @param p
 * @return
 */
template <template<typename...> class C, typename T>
std::pair<int, T> gammaQuartile(const C<T> &trace, const int quartileType, const double p)
{
    const int n = (int)trace.size();
    int j = 0;
    // We use jFloor which is the floor value of j but in the original double type
    // because when we cacul g in the 3 first cases we need the double format
    T jFloor (0);

    T m (0);
    T g (0);
    T gamma (0);
    T k (0);


    switch (quartileType) {
    // Case 1 to 3 are discontinuous case
    case 1: // It is different to R but it is identique to QuantileType1 in the article "Quantile calculations in R"
        // http://tolstoy.newcastle.edu.au/R/e17/help/att-1067/Quartiles_in_R.pdf
        m = 0.;
        jFloor = floor((n * p) + m);
        j = (int)jFloor;
        g = n*p + m - jFloor;
        gamma = (g<1e-10 ? 0 : 1.) ;
        break;

    case 2: // same probleme as type 1
        m = 0.;
        jFloor = floor((n * p) + m);
        j = (int)jFloor;
        g = n*p + m - jFloor;
        gamma = (g==0. ? 0.5 : 1.) ;
        break;

    case 3: // OK with R
        m = -0.5;
        jFloor = floor((n * p) + m);
        j = (int)jFloor;
        g = n*p + m - jFloor;
        gamma = (g==0. && isEven(j) ? 0. : 1.);
        break;

    // Case 4 to 9 are continuous case
    case 4: // OK with R
        m = 0.;
        k = p * n;
        g = k - floor(k);
        j = (int) floor(k);
        gamma = g ;
        break;
    case 5: // OK with R
        m = 0.;
        k = (p * n) + 0.5;
        g = k - floor(k);
        j = (int) floor(k);
        gamma = g ;
        break;
    case 6:
        k = p * (n+1);
        g = k - floor(k);
        j = (int) floor(k);
        gamma = g;
        break;
    case 7: // OK with R, this is the default type in R software
        k = (p*(n-1) + 1);
        g = k - floor(k);
        j = (int) floor(k);
        gamma = g ;
        break;
    case 8: // OK with R, it is the formula with Bos-Levenbach (1953) parameter
        // http://www.barringer1.com/wa_files/The-plotting-of-observations-on-probability-paper.pdf
        k = p * (n + 0.4) + 0.3;
        g = k - floor(k);
        j = (int) floor(k);
        gamma = g ;
        break;
    case 9:
        k = p * (n + 2./8.) + 3./8.;
        g = k - floor(k);
        j = (int) floor(k);
        gamma = g;
        break;

    default:
        gamma = 0.;
        break;
    }

    return qMakePair(j, gamma);
}

/**
 * @brief Compute quartile according to a type defined in R software
 * @param quartileType the type defined in R
 * @param p is the confidence must be between [0 , 1]
 * @return Q1(confidance) Q2(50%) Q3(1-confidance)
 */
template <template<typename...> class C, typename T>
Quartiles quantilesType(const C<T>& trace, const int quartileType, const double p)
{
    Quartiles Q;
    C<T> traceSorted (trace);

    decltype(gammaQuartile(trace, quartileType, p)) parQ1 = gammaQuartile(trace, quartileType, p); // first is j and second is gamma
    decltype(gammaQuartile(trace, quartileType, p)) parQ2 = gammaQuartile(trace, quartileType, 0.5);
    decltype(gammaQuartile(trace, quartileType, p)) parQ3 = gammaQuartile(trace, quartileType, 1-p);

    std::sort(traceSorted.begin(), traceSorted.end());

    // Q1 determination
    if (parQ1.first <= 0)
       Q.Q1 = (double)traceSorted.front();

    else if (parQ1.first < (int)traceSorted.size())
            Q.Q1 = (1.- parQ1.second)*(double)traceSorted.at(parQ1.first-1) + parQ1.second*(double)traceSorted.at(parQ1.first);
    else
        Q.Q1 = (double)traceSorted.back();

    // Q2 determination
    if (parQ2.first <= 0)
       Q.Q2 = (double)traceSorted.front();

    else if (parQ2.first < (int)traceSorted.size())
            Q.Q2 = (1.- parQ2.second)*(double)traceSorted.at(parQ2.first-1) + parQ2.second*(double)traceSorted.at(parQ2.first);
    else
        Q.Q2 = (double)traceSorted.back();

    // Q3 determination
    if (parQ3.first <= 0)
       Q.Q3 = (double)traceSorted.front();

    else if (parQ3.first < (int)traceSorted.size())
            Q.Q3 = (1.- parQ3.second)*(double)traceSorted.at(parQ3.first-1) + parQ3.second*(double)traceSorted.at(parQ3.first);
    else
        Q.Q3 = (double)traceSorted.back();

    return Q;
}

std::pair<double, double> solve_quadratic(const double y, const double a, const double b, const double c);

std::vector<double> gaussian_filter(std::vector<double>& curve_input, const double sigma, const short padding_type=1);
std::vector<long double> gaussian_filter(std::vector<long double>& curve_input, const double sigma, const short padding_type=1);
QMap<double, double> gaussian_filter(QMap<double, double> &map, const double sigma);

QMap<double, double> gaussian_filter_simple(const QMap<double, double> &map, const double sigma);

std::vector<double> low_pass_filter(std::vector<double>& curve_input, const double Tc, const short padding_type = 0);


#endif
