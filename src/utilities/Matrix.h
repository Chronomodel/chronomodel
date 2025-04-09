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

#ifdef DEBUG
#include <iomanip>
#include <iostream>
#endif

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
void showVector(const std::vector<double>& m, const std::string& str="");

/**
 * @class BandedMatrix
 * @brief Implementation of a banded matrix with optimized storage
 *
 * This class represents an n×n square banded matrix where only elements
 * within a specified bandwidth around the main diagonal are stored.
 * Storage is optimized by keeping only potentially non-zero elements.
 *
 * The internal storage structure uses a 1D array that stores the matrix bands
 * in a compact format. For each row i, the stored elements range from j=i-bandWidth
 * to j=i+bandWidth (within matrix bounds).
 */
class BandedMatrix {
public:
    /**
     * @brief Constructor for the banded matrix
     *
     * @param n Dimension of the matrix (n×n)
     * @param bandWidth Number of bands on each side of the main diagonal
     * @param defaultValue Default value to initialize all elements
     */
    BandedMatrix(size_t n, size_t bandWidth, long double defaultValue = 0.0)
        : m_n(n), m_bandWidth(bandWidth), m_data(n * (2 * bandWidth + 1), defaultValue) {}

    /**
     * @brief Returns the dimension of the matrix
     *
     * @return size_t Dimension n of the square matrix (n×n)
     */
    size_t getDimension() const {
        return m_n;
    }

    /**
     * @brief Returns the number of bands on each side of the main diagonal
     *
     * @return size_t Band width
     */
    size_t getBandWidth() const {
        return m_bandWidth;
    }

    /**
     * @brief Read/write access to an element via the at method
     *
     * @param i Row index
     * @param j Column index
     * @return long double& Reference to the element at position (i,j)
     * @note Indices outside the band are not checked and may cause invalid access
     */
    long double& at(size_t i, size_t j) {
        size_t offset = m_bandWidth + static_cast<int>(j - i);
        return m_data[i * (2 * m_bandWidth + 1) + offset];
    }

    /**
     * @brief Read-only access to an element via the at method
     *
     * @param i Row index
     * @param j Column index
     * @return const long double& Constant reference to the element at position (i,j)
     * @note Indices outside the band are not checked and may cause invalid access
     */
    const long double& at(size_t i, size_t j) const {
        size_t offset = m_bandWidth + static_cast<int>(j - i);
        return m_data[i * (2 * m_bandWidth + 1) + offset];
    }

    /**
     * @brief Overloaded () operator for read/write access
     *
     * @param i Row index
     * @param j Column index
     * @return long double& Reference to the element at position (i,j)
     */
    long double& operator() (size_t i, size_t j) {
        size_t offset = m_bandWidth + static_cast<int>(j - i);
        return m_data[i * (2 * m_bandWidth + 1) + offset];
    }

    /**
     * @brief Overloaded () operator for read-only access
     *
     * @param i Row index
     * @param j Column index
     * @return const long double& Constant reference to the element at position (i,j)
     */
    const long double& operator()(size_t i, size_t j) const {
        size_t offset = m_bandWidth + static_cast<int>(j - i);
        return m_data[i * (2 * m_bandWidth + 1) + offset];
    }

    /**
     * @brief Static method to create a new banded matrix
     *
     * @param n Dimension of the matrix (n×n)
     * @param bandWidth Number of bands on each side of the main diagonal
     * @param defaultValue Default value to initialize all elements
     * @return BandedMatrix New initialized banded matrix instance
     */
    static BandedMatrix initMatrix(size_t n, size_t bandWidth, long double defaultValue = 0.0) {
        return BandedMatrix(n, bandWidth, defaultValue);
    }

    /**
     * @brief Displays the matrix to standard output (available only in DEBUG mode)
     *
     * This method displays the complete matrix with zeros for elements
     * outside the band. It is conditioned by the DEBUG macro.
     */
    void showMatrix() const {
#ifdef DEBUG
        for (size_t i = 0; i < m_n; ++i) {
            for (size_t j = 0; j < m_n; ++j) {
                if (abs(static_cast<int>(j) - static_cast<int>(i)) <= static_cast<int>(m_bandWidth)) {
                    std::cout << std::setw(10) << std::fixed << std::setprecision(2) << at(i, j);
                } else {
                    std::cout << std::setw(10) << "0.00";
                }
            }
            std::cout << std::endl;
        }
#endif
    }

private:
    size_t m_n;        ///< Dimension of the matrix (n×n)
    size_t m_bandWidth; ///< Number of bands on each side of the main diagonal
    std::vector<long double> m_data; ///< Compact storage of matrix elements
};


#endif // MATRIX_H
