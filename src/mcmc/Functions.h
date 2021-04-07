/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2020

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
#include "StdUtilities.h"
#include "DateUtils.h"

#include <QMap>
#include <QVector>
#include <cmath>

typedef double type_data;

struct FunctionAnalysis{
    type_data max = (type_data)0.;
    type_data mode = (type_data)0.;
    type_data mean = (type_data)0.;
    type_data stddev = (type_data)0.;
};

struct Quartiles{
    type_data Q1 = (type_data)0.;
    type_data Q2 = (type_data)0.;
    type_data Q3 = (type_data)0.;
};

struct DensityAnalysis
{
    Quartiles quartiles;
    FunctionAnalysis analysis;
};

FunctionAnalysis analyseFunction(const QMap<type_data, type_data>& aFunction);

QString functionAnalysisToString(const FunctionAnalysis& analysis, const bool forCSV = false);
QString densityAnalysisToString(const DensityAnalysis& analysis, const QString& nl = "<br>", const bool forCSV = false);

// Standard Deviation of a vector of data
type_data dataStd(const QVector<type_data> &data);

double shrinkageUniform(const double so2);

Quartiles quartilesForTrace(const QVector<type_data>& trace);

QVector<double> calculRepartition (const QVector<double>& calib);
QVector<double> calculRepartition (const QMap<double, double> &calib);
Quartiles quartilesForRepartition(const QVector<double> &repartition, const double tmin, const double step);
QPair<double, double> credibilityForTrace(const QVector<double>& trace, double thresh, double& exactThresholdResult, const QString description = "Credibility computation");
QPair<double, double> credibilityForTrace(const QVector<int>& trace, double thresh, double& exactThresholdResult, const QString description = "Credibility computation");
QPair<double, double> timeRangeFromTraces(const QVector<double>& trace1, const QVector<double>& trace2, const double thresh, const QString description ="Time Range Computation");


QPair<double, double> gapRangeFromTraces(const QVector<double>& trace1, const QVector<double>& trace2, const double thresh, const QString description ="Gap Range Computation");

QPair<double, double> transitionRangeFromTraces(const QVector<double> &trace1, const QVector<double> &trace2, const double thresh, const QString description ="Gap Range Computation");

QString intervalText(const QPair<double, QPair<double, double> >& interval, DateConversion conversionFunc = nullptr, const bool forCSV = false);
QString getHPDText(const QMap<double, double>& hpd, double thresh, const QString& unit = QString(), DateConversion conversionFunc = nullptr, const bool forCSV =false);

QList<QPair<double, QPair<double, double> > > intervalsForHpd(const QMap<double, double> &hpd, double thresh);


//-------- Matrix
std::vector<double> initVector(const unsigned n);
std::vector<std::vector<double>> initMatrix(const unsigned rows, const unsigned cols);
void resizeMatrix(std::vector<std::vector<double>>&matrix,  const unsigned rows, const unsigned cols);

std::vector<long double> initLongVector(const unsigned n);
std::vector<std::vector<long double>> initLongMatrix(const unsigned rows, const unsigned cols);

long double determinant(const std::vector<std::vector<long double>>& matrix);


std::vector<std::vector<long double> > transpose0(const std::vector<std::vector<long double>>& matrix);
std::vector<std::vector<long double>> transpose(const std::vector<std::vector<long double>>& matrix, const int nbDiag);
std::vector<std::vector<long double>> multiMatParDiag(const std::vector<std::vector<long double>>& matrix, const std::vector<long double>& diag, const int nbBandes);
std::vector<std::vector<long double>> multiDiagParMat(const std::vector<long double>& diag, const std::vector<std::vector<long double>>& matrix, const int nbBandes);
std::vector<long double> multiMatParVec(const std::vector<std::vector<long double>>& matrix, const std::vector<long double>& vec, const int nbBandes);

std::vector<std::vector<long double>> addMatEtMat0(const std::vector<std::vector<long double>>& matrix1, const std::vector<std::vector<long double>>& matrix2);
std::vector<std::vector<long double>> addMatEtMat(const std::vector<std::vector<long double>>& matrix1, const std::vector<std::vector<long double>>& matrix2, const int nbBandes);
std::vector<std::vector<long double>> addIdentityToMat(const std::vector<std::vector<long double>>& matrix);
std::vector<std::vector<long double>> multiConstParMat(const std::vector<std::vector<long double>>& matrix, const double c, const int nbBandes);

std::vector<std::vector<long double> > multiMatParMat0(const std::vector<std::vector<long double> > &matrix1, const std::vector<std::vector<long double> > &matrix2);
std::vector<std::vector<long double>> multiMatParMat(const std::vector<std::vector<long double>>& matrix1, const std::vector<std::vector<long double>>& matrix2, const int nbBandes1, const int nbBandes2);

std::vector<std::vector<long double>> inverseMatSym0(const std::vector<std::vector<long double>>& matrix);
std::vector<std::vector<long double>> inverseMatSym(const std::vector<std::vector<long double>>& matrix1, const std::vector<long double>& matrix2, const int nbBandes, const int shift);
long double sumAllMatrix(const std::vector<std::vector<long double>>& matrix);
long double sumAllVector(const std::vector<long double>& matrix);

std::vector<std::vector<long double>> cofactor0(const std::vector<std::vector<long double> > &matrix);
std::vector<std::vector<long double> > comatrice0(const std::vector<std::vector<long double> > &matrix);


std::vector<std::vector<long double>> choleskyLL0(const std::vector<std::vector<long double>>& matrix);
std::pair<std::vector<std::vector<long double> >, std::vector<long double> > choleskyLDLT(const std::vector<std::vector<long double>>& matrix);
std::pair<std::vector<std::vector<long double> >, std::vector<long double> > decompositionCholesky(const std::vector<std::vector<long double> > &matrix, const int nbBandes, const int shift);

std::pair<std::vector<std::vector<long double> >, std::vector<std::vector<long double> > > decompositionLU0(const std::vector<std::vector<long double> > &A);

std::vector<long double> resolutionSystemeLineaireCholesky(const std::vector<std::vector<long double> >& matL, const std::vector<long double>& matD, const std::vector<long double>& vecQtY);

struct Strassen
{ //https://www.sanfoundry.com/java-program-strassen-algorithm/

    std::vector<std::vector<long double>> multiply (const std::vector<std::vector<long double>>& A, const std::vector<std::vector<long double>>& B);
std::vector<std::vector<long double>> sub(const std::vector<std::vector<long double>>&  A, const std::vector<std::vector<long double>>& B);
  std::vector<std::vector<long double>> add(const std::vector<std::vector<long double>>& A, const std::vector<std::vector<long double>>& B);
/** Funtion to split parent matrix into child matrices **/

void split(const std::vector<std::vector<long double>>& P, std::vector<std::vector<long double> > &C, int iB, int jB) ;
/** Funtion to join child matrices intp parent matrix **/

 void join(const std::vector<std::vector<long double>>& C, std::vector<std::vector<long double> > &P, int iB, int jB) ;

};

inline double rounddouble(const double f,const int prec)
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
bool isOdd( T value )
{
    return (value % 2!= 0 ? true : false);
}

template <typename T>
bool isEven( T value )
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
template <typename T>
QPair<int, double> gammaQuartile(const QVector<T> &trace, const int quartileType, const double p)
{
    const int n (trace.size());
    int j (0);
    // We use jFloor which is the floor value of j but in the original double type
    // because when we cacul g in the 3 first cases we need the double format
    double jFloor(0.);

    double m (0.);
    double g (0.);
    double gamma (0.);
    double k (0.);


    switch (quartileType) {
    // Case 1 to 3 are discontinuous case
    case 1: // It is different to R but it is identique to QuantileType1 in the article "Quantile calculations in R"
        // http://tolstoy.newcastle.edu.au/R/e17/help/att-1067/Quartiles_in_R.pdf
        m = 0.;
        jFloor = floor((n * p) + m);
        j = (int)jFloor;
        g = n*p + m - jFloor;

        gamma = (g<1e-10 ? 0 : 1.) ;
 //qDebug()<<n<<p<<m<<jFloor<<j<<g<<gamma;
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
template <typename T>
Quartiles quartilesType(const QVector<T>& trace, const int quartileType, const double p)
{
    Q_ASSERT(&trace);
    Quartiles Q;
    QVector<T> traceSorted (trace);

    QPair<int, double> parQ1 = gammaQuartile(trace, quartileType, p); // first is j and second is gamma
    QPair<int, double> parQ2 = gammaQuartile(trace, quartileType, 0.5);
    QPair<int, double> parQ3 = gammaQuartile(trace, quartileType, 1-p);

    std::sort(traceSorted.begin(), traceSorted.end());

    // Q1 determination
    if (parQ1.first<=0)
       Q.Q1 = (double)traceSorted.first();

    else if (parQ1.first < traceSorted.size())
            Q.Q1 = (1.- parQ1.second)*(double)traceSorted.at(parQ1.first-1) + parQ1.second*(double)traceSorted.at(parQ1.first);
    else
        Q.Q1 = (double)traceSorted.last();

    // Q2 determination
    if (parQ2.first<=0)
       Q.Q2 = (double)traceSorted.first();

    else if (parQ2.first < traceSorted.size())
            Q.Q2 = (1.- parQ2.second)*(double)traceSorted.at(parQ2.first-1) + parQ2.second*(double)traceSorted.at(parQ2.first);
    else
        Q.Q2 = (double)traceSorted.last();

    // Q3 determination
    if (parQ3.first<=0)
       Q.Q3 = (double)traceSorted.first();

    else if (parQ3.first < traceSorted.size())
            Q.Q3 = (1.- parQ3.second)*(double)traceSorted.at(parQ3.first-1) + parQ3.second*(double)traceSorted.at(parQ3.first);
    else
        Q.Q3 = (double)traceSorted.last();

    return Q;
}


#endif
