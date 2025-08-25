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
#include <iostream>
#endif

#include <vector>
#include <valarray>
#include <iostream>
#include <stdexcept>

#include <QDataStream>
#include <QtDebug>

#include <eigen_3.4.0/Eigen/Dense>
#include <eigen_3.4.0/Eigen/src/Core/DiagonalMatrix.h>

/**
 * @class CurveMap
 * @brief Représente une carte 2D de données numériques avec gestion des axes X/Y.
 *
 * Les données sont stockées dans une `std::valarray<double>` de taille `_row * _column`,
 * indexées selon `data[column * _row + row]`. L'accès se fait via `operator()` ou `at()`.
 */

#include <utility>
#include <QDebug>

typedef double t_reduceTime;
typedef long double t_matrix;

using namespace Eigen;
typedef Matrix<t_matrix, Dynamic, Dynamic> Matrix2D;

using DiagonalMatrixLD = DiagonalMatrix<t_matrix, Dynamic, Dynamic>;

typedef Eigen::Matrix<t_matrix, Eigen::Dynamic, 1> ColumnVectorLD;
typedef Eigen::Matrix<t_matrix, 1, Eigen::Dynamic> RowVectorLD;

ColumnVectorLD stdVectorToColumnVector(const std::vector<double>& vec);
ColumnVectorLD stdVectorToColumnVector(const std::vector<t_matrix>& vec);
std::vector<t_matrix> eigenToStdVector(const ColumnVectorLD& vec);

typedef Eigen::Matrix<t_matrix, 1, Eigen::Dynamic> RowVectorLD;
RowVectorLD stdVectorToRowVector(const std::vector<double>& vec);
RowVectorLD stdVectorToRowVector(const std::vector<t_matrix>& vec);
std::vector<t_matrix> eigenToStdVector(const RowVectorLD& vec);


class CurveMap
{
public:
    using iterator = std::vector<double>::iterator;
    using const_iterator = std::vector<double>::const_iterator;

    unsigned _row, _column;
    std::vector<double> data;
    std::pair<double, double> rangeY;
    std::pair<double, double> rangeX;

    double min_value;
    double max_value;

    CurveMap()
        : _row(0), _column(0), data(), min_value(0), max_value(0) {
        rangeX = std::make_pair(0, 0);
        rangeY = std::make_pair(0, 0);
    }

    explicit CurveMap(unsigned row, unsigned col)
        : _row(row), _column(col), data(row * col, 0.0), min_value(0), max_value(0) {}

    void setRangeX(double minX, double maxX) { rangeX = std::make_pair(minX, maxX); }
    void setRangeY(double minY, double maxY) { rangeY = std::make_pair(minY, maxY); }

    double& at(unsigned c, unsigned r) {
        try {
            return data[_row * c + r];
        } catch (...) {
            qDebug() << "CurveMap over ???" << c << r;
            return data[0];
        }
    }

    double& operator()(unsigned c, unsigned r) { return data[_row * c + r]; }
    const double& operator()(unsigned c, unsigned r) const { return data[_row * c + r]; }

    // Pour comportement style std::vector[i]
    double& operator[](size_t idx) { return data[idx]; }
    const double& operator[](size_t idx) const { return data[idx]; }

    // Incrémentation d'une cellule
    void increment(unsigned col, unsigned row) {
        at(col, row) += 1.0;
    }


    // Ajout d'une valeur dans une cellule
    void operator+=(std::pair<unsigned, unsigned> coord_val) {
        at(coord_val.first, coord_val.second) += 1.0;
    }

    // Fournir un pointeur direct à la cellule
    double* dataPtr(unsigned c, unsigned r) {
        return &data[_row * c + r];
    }

    unsigned row() const { return _row; }
    unsigned column() const { return _column; }

    double minX() const { return rangeX.first; }
    double maxX() const { return rangeX.second; }
    double minY() const { return rangeY.first; }
    double maxY() const { return rangeY.second; }

    void clear() {
        data.clear();
        _row = _column = 0;
    }

    iterator begin() { return data.begin(); }
    iterator end() { return data.end(); }
    const_iterator begin() const { return data.begin(); }
    const_iterator end() const { return data.end(); }

    virtual ~CurveMap() = default;
};

#if false
class CurveMap
{
public:
    /// Nombre de lignes
    unsigned _row;

    /// Nombre de colonnes
    unsigned _column;

    /// Données de la carte, stockées en ligne continue
    std::valarray<double> data;

    /// Intervalle des valeurs de l’axe Y
    std::pair<double, double> rangeY;

    /// Intervalle des valeurs de l’axe X
    std::pair<double, double> rangeX;

    /// Valeur minimale des données (peut être mise à jour via updateMinMax)
    double min_value;

    /// Valeur maximale des données (peut être mise à jour via updateMinMax)
    double max_value;

    /**
     * @brief Constructeur par défaut.
     */
    CurveMap()
        : _row(0), _column(0), data(), rangeY({0, 0}), rangeX({0, 0}),
        min_value(0), max_value(0) {}

    /**
     * @brief Constructeur avec dimensions.
     * @param row Nombre de lignes
     * @param col Nombre de colonnes
     */
    explicit CurveMap(unsigned row, unsigned col)
        : _row(row), _column(col), data(row * col), rangeY({0, 0}), rangeX({0, 0}),
        min_value(0), max_value(0) {}


    /// Définit l’intervalle de l’axe X
    void setRangeX(double minX, double maxX) { rangeX = std::make_pair(minX, maxX); }

    /// Définit l’intervalle de l’axe Y
    void setRangeY(double minY, double maxY) { rangeY = std::make_pair(minY, maxY); }

    /**
     * @brief Accès sécurisé à un élément.
     * @param c Colonne
     * @param r Ligne
     * @return Référence vers la valeur
     */
    double& at(unsigned c, unsigned r) {
        size_t idx = index(c, r);
        if (idx >= data.size()) {
            qDebug() << "[CurveMap] out-of-bounds access: (" << c << "," << r << ")";
            return data[0];  // fail-safe
        }
        return data[idx];
    }

    /// Accès en écriture à un élément via ()
    double& operator()(unsigned c, unsigned r) { return data[index(c, r)]; }

    /// Accès en lecture à un élément via ()
    const double& operator()(unsigned c, unsigned r) const { return data[index(c, r)]; }

    /// Accès pointeur vers une cellule
    double* ptr_at(unsigned c, unsigned r) { return &data[index(c, r)]; }

    /// Accès en lecture (linéaire, comme std::vector)
    const double& operator[](size_t index) const {
        return data[index];
    }

    /// Accès en écriture (linéaire, comme std::vector)
    double& operator[](size_t index) {
        return data[index];
    }

    /// Nombre de lignes
    unsigned row() const { return _row; }

    /// Nombre de colonnes
    unsigned column() const { return _column; }

    /// Minimum de l’axe X
    double minX() const { return rangeX.first; }

    /// Maximum de l’axe X
    double maxX() const { return rangeX.second; }

    /// Minimum de l’axe Y
    double minY() const { return rangeY.first; }

    /// Maximum de l’axe Y
    double maxY() const { return rangeY.second; }

    /// Vide les données
    void clear() {
        data.resize(0);
        _row = 0;
        _column = 0;
        min_value = max_value = 0;
    }

    /// Redimensionne la carte
    void resize(unsigned row, unsigned col) {
        _row = row;
        _column = col;
        data.resize(row * col);
        min_value = max_value = 0;
    }

    /**
     * @brief Met à jour les valeurs min et max à partir des données.
     */
    void updateMinMax() {
        if (data.size() == 0) {
            min_value = max_value = 0;
            return;
        }
        min_value = data.min();
        max_value = data.max();
    }

    /// Destructeur virtuel
    virtual ~CurveMap() = default;

private:
    /// Calcule l’index linéaire pour accéder à la cellule (c, r)
    size_t index(unsigned c, unsigned r) const {
        return _row * c + r;
    }
};
#endif
//typedef std::valarray<std::valarray<t_matrix>> Matrix2D;


class Matrix2Do {
private:
    std::valarray<std::valarray<t_matrix>> data;
    size_t n;
    size_t m; // Nombre de colonnes

public:
    // Constructeur par defaut
    Matrix2Do() : data(std::valarray<t_matrix>(0), 0), n(0), m(0) {}

    // Constructeur pour une matrice de dimensions n x m
    Matrix2Do(size_t rows, size_t cols) : data(std::valarray<t_matrix>(0.0L, cols), rows), n(rows), m(cols) {}

    // Constructeur pour une matrice identité (carrée)
    Matrix2Do(size_t size, bool identity) : data(std::valarray<t_matrix>(0.0L, size), size), n(size), m(size) {
        if (identity) {
            for (size_t i = 0; i < size; ++i) {
                data[i][i] = 1.0L;
            }
        }
    }

    // Destructeur : inutile de faire quoi que ce soit, le compilateur s'en charge
    virtual ~Matrix2Do() = default;


    static Matrix2D initMatrix2D(size_t rows, size_t cols) {
        return Matrix2D(rows, cols);
    }

    // Accesseurs
    std::valarray<t_matrix>& operator[](size_t i) { return data[i]; }
    const std::valarray<t_matrix>& operator[](size_t i) const { return data[i]; }
    size_t rows() const { return n; }
    size_t cols() const { return m; }

    // Opérateur d'addition
    Matrix2Do operator+(const Matrix2Do& other) const;
    // Opérateur de soustraction
    Matrix2Do operator-(const Matrix2Do& other) const;
    // Opérateur de multiplication matricielle
    Matrix2Do operator*(const Matrix2Do& other) const;
    // Opérateur de multiplication par un scalaire
    Matrix2Do operator*(t_matrix scalar) const;

    // Opérateur de multiplication par un vecteur
    std::valarray<t_matrix> operator*(const std::valarray<t_matrix>& vec) const;
    std::vector<t_matrix> operator*(const std::vector<t_matrix>& vec) const;


    // Surcharge de l'opérateur d'itération pour permettre l'utilisation de boucles range-based
    std::valarray<t_matrix>* begin();

    std::valarray<t_matrix>* end();

    // Surcharge de l'opérateur unaire pour permettre l'inversion des signes
    Matrix2Do operator-() const;
    // Méthode d'affichage
    void showMatrix(const std::string& str = "") const;
    // Méthode pour échanger deux lignes
    void swapRows(int i, int j);
    /**
     * @brief Transpose the matrix
     */
    Matrix2Do transpose() const;

    // Méthode pour inverser la matrice
    Matrix2Do inverse() const;
};


QDataStream &operator<<( QDataStream& stream, const CurveMap& map );
QDataStream &operator>>( QDataStream& stream, CurveMap& map );

#pragma mark Usefull to debug

void showVector(const std::vector<t_matrix>& m, const std::string& str = "");
void showVector(const std::vector<double>& m, const std::string& str = "");
void showMatrix(const Matrix2D& m, const std::string& str = "");
void showMatrix(const DiagonalMatrixLD& d, const std::string& str = "");
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

private:
    size_t m_n;
    size_t m_m; // Nombre de colonnes
    size_t m_bandWidth; ///< Number of bands on each side of the main diagonal
    std::vector<t_matrix> m_data; ///< Compact storage of matrix elements


protected:
    std::vector<t_matrix> data()const {
        return m_data;
    }

    bool inBand(size_t i, size_t j) const {
        return (i < m_n && j < m_m && std::abs(static_cast<int>(j) - static_cast<int>(i)) <= static_cast<int>(m_bandWidth));
    }

public:
    BandedMatrix() : m_n(0), m_m(0), m_bandWidth(0), m_data(0) {}

    BandedMatrix(size_t n, size_t m, size_t bandwidth, t_matrix defaultValue = 0.0)
        : m_n(n), m_m(m), m_bandWidth(bandwidth),
        m_data(n * (2 * bandwidth + 1), defaultValue) {}

    explicit BandedMatrix(size_t n, size_t m, size_t bandwidth, std::vector<t_matrix> data)
        : m_n(n), m_m(m), m_bandWidth(bandwidth),  m_data(data) {}

    virtual ~BandedMatrix() = default;
    /**
     * @brief Read/write access to an element via the at method
     *
     * @param i Row index
     * @param j Column index
     * @return long double& Reference to the element at position (i,j)
     * @note Indices outside the band are not checked and may cause invalid access
     */
    t_matrix& at(size_t i, size_t j) {
        if (!inBand(i, j)) {
            throw std::out_of_range("Access outside band");
        }
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
    const t_matrix& at(size_t i, size_t j) const {
        if (!inBand(i, j)) {
            static const t_matrix zero = 0.0L;
            return zero;
        }
        size_t offset = m_bandWidth +  static_cast<int>(j - i);
        return m_data[i * (2 * m_bandWidth + 1) + offset];
    }

    size_t rows() const { return m_n; }
    size_t cols() const { return m_m; }

    std::vector<t_matrix>::iterator begin() {
        return m_data.begin();
    }

    std::vector<t_matrix>::iterator end() {
        return m_data.end();
    }

    std::vector<t_matrix>::const_iterator cbegin() const {
        return m_data.cbegin();
    }

    std::vector<t_matrix>::const_iterator cend() const {
        return m_data.cend();
    }

    /**
     * @brief Copy constructor
     */
    BandedMatrix(const BandedMatrix& other)
        : m_n(other.m_n), m_bandWidth(other.m_bandWidth), m_data(other.m_data) {}

    /**
     * @brief Assignment operator
     */
    BandedMatrix& operator=(const BandedMatrix& other) {
        if (this != &other) {
            m_n = other.m_n;
            m_m = other.m_m;
            m_bandWidth = other.m_bandWidth;
            m_data = other.m_data;
        }
        return *this;
    }
    /**
     * @brief Assignment move
     */
    BandedMatrix& operator=(BandedMatrix&& other) noexcept {
        if (this != &other) {
            m_n = other.m_n;
            m_m = other.m_m;
            m_bandWidth = other.m_bandWidth;
            m_data = std::move(other.m_data);
        }
        return *this;
    }


    /**
     * @brief Returns the number of bands on each side of the main diagonal
     *
     * @return size_t Band width
     */
    inline size_t getBandWidth() const {
        return m_bandWidth;
    }


    /**
     * @brief Overloaded () operator for read/write access
     *
     * @param i Row index
     * @param j Column index
     * @return t_matrix& Reference to the element at position (i,j)
     */
    inline t_matrix& operator()(size_t i, size_t j) {
        return at(i, j);
    }

    /**
     * @brief Overloaded () operator for read-only access
     *
     * @param i Row index
     * @param j Column index
     * @return const t_matrix& Constant reference to the element at position (i,j)
     */
    inline const t_matrix& operator()(size_t i, size_t j) const {
        return at(i, j);
    }

    // ========== OPÉRATEURS ARITHMÉTIQUES ==========

    /**
     * @brief Addition operator
     */
    BandedMatrix operator+(const BandedMatrix& other) const;

    /**
     * @brief Subtraction operator
     */

    BandedMatrix operator-(const BandedMatrix& other) const;

    /**
     * @brief Matrix multiplication operator
     */
    BandedMatrix operator*(const BandedMatrix& other) const;

    /**
     * @brief Scalar multiplication operator
     */

    BandedMatrix operator*(t_matrix scalar) const;

    /**
     * @brief Vector multiplication operator (Matrix × Vector)
     */
    template<class U>
    std::vector<t_matrix> operator*(const std::vector<U>& vec) const;


    // ========== OPÉRATEURS D'ASSIGNATION ==========

    /**
     * @brief Addition assignment operator
     */
    BandedMatrix& operator+=(const BandedMatrix& other);

    /**
     * @brief Subtraction assignment operator
     */
    BandedMatrix& operator-=(const BandedMatrix& other);

    /**
     * @brief Scalar multiplication assignment operator
     */
    BandedMatrix& operator*=(long double scalar);
    /**
     * @brief Scalar division assignment operator
     */
    BandedMatrix& operator/=(long double scalar);

    // ========== OPÉRATEURS DE COMPARAISON ==========

    /**
     * @brief Equality operator
     */
    bool operator==(const BandedMatrix& other) const;

    /**
     * @brief Inequality operator
     */
    bool operator!=(const BandedMatrix& other) const;

    // ========== OPÉRATEURS UNAIRES ==========

    /**
     * @brief Unary minus operator
     */
    BandedMatrix operator-() const;

    // ========== MÉTHODES UTILITAIRES ==========

    /**
 * @brief Transpose the matrix
 */
    BandedMatrix transpose() const;

    /**
    * @brief Get the trace of the matrix
    */
    t_matrix trace() const;

    /**
    * @brief Check if the matrix is symmetric
    */
    bool isSymmetric() const;

    /**
    * @brief Static method to create a new banded matrix
    *
    * @param n Number of rows
    * @param m Number of columns
    * @param bandWidth Number of bands on each side of the main diagonal
    * @param defaultValue Default value to initialize all elements
    * @return BandedMatrix New initialized banded matrix instance
    */
    static BandedMatrix initMatrix(size_t n, size_t m, size_t bandWidth, t_matrix defaultValue = 0.0);

    /**
    * @brief Create an identity matrix (square n × n)
    *
    * @param n Matrix dimension
    * @param bandWidth Band width (optional, default is 0)
    * @return BandedMatrix Identity matrix
    */
    static BandedMatrix identity(size_t n, size_t bandWidth = 0);

    /**
    * @brief Displays the matrix to standard output
    *
    * This method displays the complete matrix with zeros for elements
    * outside the band.
    */
    void showMatrix(const std::string& str = "") const;


    /**
    * @brief Create a diagonal matrix from a vector of values
    */
    static BandedMatrix diagonal(const std::vector<t_matrix>& diag, size_t bandWidth = 0);

    // ================= DECOMPOSITION LU =====================
    // NOTE: uniquement pour matrices carrées
    void computeLU(BandedMatrix& L, BandedMatrix& U) const;

};

#pragma mark OPÉRATEURS EXTERNES
/**
 * @brief Scalar multiplication (scalar × matrix)
 */

BandedMatrix operator*(long double scalar, const BandedMatrix& matrix);

/**
 * @brief Stream output operator
 */
 std::ostream& operator<<(std::ostream& os, const BandedMatrix& matrix);





#endif // MATRIX_H
