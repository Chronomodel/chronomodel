/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2024

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

inline double dnorm (const double x, const double mu = 0., const double std = 1.) {return exp(-0.5*pow((x - mu)/ std, 2.))/ (sqrt(2.*M_PI)*std);}

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
std::vector<double> initVector(size_t n);

std::vector<std::vector<int>> initIntMatrix(size_t rows, size_t cols);
std::vector<std::vector<double>> initMatrix(size_t rows, size_t cols);
void resizeMatrix(std::vector<double> &matrix,  size_t rows, size_t cols);

std::vector<long double> initLongVector(size_t n);


Matrix2D::value_type::value_type determinant(const Matrix2D& matrix, size_t shift = 0); // à contrôler
Matrix2D::value_type::value_type determinant_gauss(const Matrix2D &matrix, size_t shift = 0);

Matrix2D seedMatrix(const Matrix2D matrix, size_t shift = 0);
Matrix2D remove_bands_Matrix(const Matrix2D& matrix, size_t shift = 0);

Matrix2D transpose0(const Matrix2D& matrix);
Matrix2D transpose(const Matrix2D& matrix, const size_t nbDiag);

Matrix2D multiMatParDiag(const Matrix2D& matrix, const MatrixDiag& diag, size_t nbBandes);
Matrix2D multiDiagParMat(const MatrixDiag& diag, const Matrix2D& matrix, const size_t nbBandes);

std::vector<double> multiMatParVec(const Matrix2D& matrix, const std::vector<double> &vec, const size_t nbBandes);
std::vector<t_matrix> multiMatParVec(const Matrix2D& matrix, const std::vector<t_matrix> &vec, const size_t nbBandes);


Matrix2D addMatEtMat0(const Matrix2D& matrix1, const Matrix2D& matrix2);
Matrix2D addMatEtMat(const Matrix2D& matrix1, const Matrix2D& matrix2, const size_t nbBandes);
Matrix2D addIdentityToMat(const Matrix2D& matrix);
Matrix2D multiConstParMat(const Matrix2D& matrix, const double c, const size_t nbBandes);
Matrix2D multiConstParMat0(const Matrix2D& matrix, const double c);

Matrix2D multiMatParMat0(const Matrix2D& matrix1, const Matrix2D& matrix2);
Matrix2D multiMatParMat(const Matrix2D& matrix1, const Matrix2D& matrix2, const size_t nbBandes1, const size_t nbBandes2);

Matrix2D addDiagToMat(const MatrixDiag& diag, Matrix2D matrix);

Matrix2D soustractMatToIdentity(const Matrix2D& matrix);

Matrix2D multiplyMatrix_Naive(const Matrix2D& a, const Matrix2D& b);
Matrix2D multiplyMatrix_Winograd(const Matrix2D& a, const Matrix2D& b);
Matrix2D multiplyMatrixBanded_Winograd(const Matrix2D& a, const Matrix2D& b,  const int bandwidth = 0);

Matrix2D inverseMatSym0(const Matrix2D& matrix, const size_t shift = 0);
Matrix2D inverseMatSym(const Matrix2D& matrix1, const MatrixDiag& matrix2, const size_t nbBandes, const size_t shift);

Matrix2D inverseMatSym_origin(const std::pair<Matrix2D, MatrixDiag>& decomp, const size_t nbBandes, const size_t shift);


double sumAllMatrix(const std::vector<std::vector<double>>& matrix);
t_matrix sumAllMatrix(const Matrix2D& m);


double sumAllVector(const std::vector<double>& matrix);

Matrix2D cofactor0(const Matrix2D& matrix);
Matrix2D comatrice0(const Matrix2D& matrix);


Matrix2D choleskyLL0(const Matrix2D& matrix);
std::pair<Matrix2D, MatrixDiag > choleskyLDLT(const Matrix2D& matrix);
std::pair<Matrix2D, MatrixDiag > choleskyLDLT(const Matrix2D& matrix, const size_t shift);
std::pair<Matrix2D, MatrixDiag > choleskyLDLT(const Matrix2D& matrix, const size_t nbBandes, const size_t shift);
std::pair<Matrix2D, MatrixDiag > choleskyLDLT_Dsup0(const Matrix2D& matrix, const size_t nbBandes, const size_t shift);
std::pair<Matrix2D, MatrixDiag> decompositionCholesky(const Matrix2D& matrix, const size_t nbBandes, const size_t shift);

std::pair<Matrix2D, Matrix2D > decompositionLU0(const Matrix2D& A);
std::pair<Matrix2D, Matrix2D> Doolittle_LU(const Matrix2D A);
std::pair<Matrix2D, MatrixDiag> LU_to_LD(const std::pair<Matrix2D, Matrix2D> LU);

std::pair<Matrix2D, Matrix2D > decompositionQR(const Matrix2D& A);
std::pair<Matrix2D, Matrix2D> householderQR(Matrix2D& A);

std::vector<double> resolutionSystemeLineaireCholesky(const std::pair<Matrix2D, MatrixDiag>& decomp, const std::vector<double>& vecQtY);
std::vector<t_matrix> resolutionSystemeLineaireCholesky(const std::pair<Matrix2D, MatrixDiag>& decomp, const std::vector<t_matrix>& vecQtY);
std::vector<long double> resolutionSystemeLineaireCholesky_long(const std::pair<Matrix2D, MatrixDiag>& decomp, const std::vector<double>& vecQtY);
struct Strassen
{ //https://www.sanfoundry.com/java-program-strassen-algorithm/

    Matrix2D multiply (const Matrix2D& A, const Matrix2D& B);
    Matrix2D sub(const Matrix2D& A, const Matrix2D& B);
    Matrix2D add(const Matrix2D& A, const Matrix2D& B);
/** Funtion to split parent matrix into child matrices **/

void split(const Matrix2D& P, Matrix2D& C, int iB, int jB) ;
/** Funtion to join child matrices intp parent matrix **/

 void join(const Matrix2D &C, Matrix2D &P, int iB, int jB) ;

};

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
    decltype(gammaQuartile(trace, quartileType, p))  parQ2 = gammaQuartile(trace, quartileType, 0.5);
    decltype(gammaQuartile(trace, quartileType, p))  parQ3 = gammaQuartile(trace, quartileType, 1-p);

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

std::vector<double> gaussian_filter(std::vector<double>& curve_input, const double sigma);

#endif
