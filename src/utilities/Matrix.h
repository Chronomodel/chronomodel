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

#ifndef MATRIX_H
#define MATRIX_H

#include <vector>
#include <valarray>

#include <QDataStream>
#include <QtDebug>



class CurveMap
{
public:
    unsigned _row, _column;
    std::valarray<double> data;
    std::pair<double, double> rangeY;
    std::pair<double, double> rangeX;

    double min_value;
    double max_value;

    CurveMap() : _row(0), _column(0), data(), min_value(0), max_value(0) {
        data = std::valarray<double> ();
        rangeX = std::make_pair(0, 0);
        rangeY = std::make_pair(0, 0);
    }
    explicit CurveMap(unsigned row, unsigned col):_row(row), _column(col), data(row * col), min_value(0), max_value(0) {}

    void setRangeX(double minX, double maxX) {rangeX = std::make_pair(minX, maxX);}
    void setRangeY(double minY, double maxY) {rangeY = std::make_pair(minY, maxY);}

    double& at(unsigned c, unsigned r) { // beCarefull invertion of row and column between definiton of CurveMap() and at()
        try {
            return data[_row*c + r];
        }  catch (...) {
            qDebug() <<"CurveMap over ???"<<c << r;
            return data[0];
        }

    }
    double& operator()(unsigned c, unsigned r) { return data[_row*c + r];} //Set
    const double& operator()(unsigned c, unsigned r)  const { return data[_row*c + r];} //Get
    double* ptr_at(unsigned c, unsigned r) {return (begin(data) + (_row*c + r));}

    unsigned row() const {return _row;}
    unsigned column() const {return _column;}

    double minX() const {return rangeX.first;}
    double maxX() const {return rangeX.second;}
    double minY() const {return rangeY.first;}
    double maxY() const {return rangeY.second;}
    void clear() {data.resize(0);}
    virtual ~CurveMap();

};

typedef double t_reduceTime;
typedef long double t_matrix;
typedef std::valarray<std::valarray<t_matrix>> Matrix2D;

typedef std::vector<t_matrix> MatrixDiag;

Matrix2D initMatrix2D(size_t rows, size_t cols);

QDataStream &operator<<( QDataStream& stream, const CurveMap& map );
QDataStream &operator>>( QDataStream& stream, CurveMap& map );

#pragma mark Usefull to debug
void showMatrix(const Matrix2D& m, const std::string& str="");
void showMatrix(const MatrixDiag& m, const std::string& str="");

void showVector(const std::vector<t_matrix>& m, const std::string& str="");

#endif // MATRIX_H
