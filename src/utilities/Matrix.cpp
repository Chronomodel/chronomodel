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

#include "Matrix.h"

#include <iomanip>
#include <iostream>
#include <vector>

ColumnVectorLD stdVectorToColumnVector(const std::vector<double>& vec)
{
    std::vector<t_matrix> v (vec.begin(), vec.end());
    return Eigen::Map<const ColumnVectorLD>(v.data(), v.size());
}

ColumnVectorLD stdVectorToColumnVector(const std::vector<t_matrix>& vec)
{
    return Eigen::Map<const ColumnVectorLD>(vec.data(), vec.size());
}

std::vector<t_matrix> eigenToStdVector(const ColumnVectorLD& vec)
{
    return std::vector<t_matrix>(vec.data(), vec.data() + vec.size());
}

RowVectorLD stdVectorToRowVector(const std::vector<double>& vec)
{
    std::vector<t_matrix> v (vec.begin(), vec.end());
    return Eigen::Map<const RowVectorLD>(v.data(), v.size());
}

RowVectorLD stdVectorToRowVector(const std::vector<t_matrix>& vec)
{
    return Eigen::Map<const RowVectorLD>(vec.data(), vec.size());
}

std::vector<t_matrix> eigenToStdVector(const RowVectorLD& vec)
{
    return std::vector<t_matrix>(vec.data(), vec.data() + vec.size());
}

#pragma mark Matrix2Do
// Opérateur d'addition
Matrix2Do Matrix2Do::operator+(const Matrix2Do& other) const {
    if (n != other.n || m != other.m) {
        throw std::invalid_argument("Dimensions mismatch for addition");
    }
    Matrix2Do result(n, m);
    for (size_t i = 0; i < n; i++) {
        result.data[i] = data[i] + other.data[i];
    }
    return result;
}

// Opérateur de soustraction
Matrix2Do Matrix2Do::operator-(const Matrix2Do& other) const {
    if (n != other.n || m != other.m) {
        throw std::invalid_argument("Dimensions mismatch for addition");
    }
    Matrix2Do result(n, m);
    for (size_t i = 0; i < n; i++) {
        result.data[i] = data[i] - other.data[i];
    }
    return result;
}

// Opérateur de multiplication matricielle


Matrix2Do Matrix2Do::operator*(const Matrix2Do& other) const {
    if (m != other.n) {
        throw std::invalid_argument("Dimensions mismatch for multiplication");
    }

    Matrix2Do result(n, other.m);

    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < other.m; j++) {
            result.data[i][j] = std::inner_product(std::begin(data[i]), std::end(data[i]),
                                                   std::begin(other.data[0]) + j, 0.0L,
                                                   std::plus<t_matrix>(),
                                                   std::multiplies<t_matrix>());
        }
    }

    return result;
}


// Opérateur de multiplication par un scalaire
Matrix2Do Matrix2Do::operator*(t_matrix scalar) const {
    Matrix2Do result(n, m);
    for (size_t i = 0; i < n; i++) {
        result.data[i] = data[i] * scalar;
    }
    return result;
}

// Opérateur de multiplication par un vecteur
std::valarray<t_matrix> Matrix2Do::operator*(const std::valarray<t_matrix>& vec) const {
    if (m != vec.size()) {
        throw std::invalid_argument("Dimensions mismatch for vector multiplication");
    }
    std::valarray<t_matrix> result(n);
    for (size_t i = 0; i < n; i++) {
        t_matrix sum = 0.0L;
        for (size_t j = 0; j < m; j++) {
            sum += data[i][j] * vec[j];
        }
        result[i] = sum;
    }
    return result;
}

std::vector<t_matrix> Matrix2Do::operator*(const std::vector<t_matrix>& vec) const {
    if (m != vec.size()) {
        throw std::invalid_argument("Dimensions mismatch for vector multiplication");
    }
    std::vector<t_matrix> result(n);
    for (size_t i = 0; i < n; i++) {
        t_matrix sum = 0.0L;
        for (size_t j = 0; j < m; j++) {
            sum += data[i][j] * vec[j];
        }
        result[i] = sum;
    }
    return result;
}


// Surcharge de l'opérateur d'itération pour permettre l'utilisation de boucles range-based
std::valarray<t_matrix>* Matrix2Do::begin() {
    return std::begin(data);
}

std::valarray<t_matrix>* Matrix2Do::end() {
    return std::end(data);
}

// Surcharge de l'opérateur unaire pour permettre l'inversion des signes
Matrix2Do Matrix2Do::operator-() const {
    Matrix2Do result(*this);
    for (auto&& r_i : result)
        r_i *= -1.;
    return result;
}
// Méthode d'affichage
void Matrix2Do::showMatrix(const std::string& str) const {
    std::cout << str << std::endl;
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < m; j++) {
            std::cout << std::setw(10) << std::fixed << std::setprecision(6) << data[i][j] << " ";
        }
        std::cout << std::endl;
    }
}
// Méthode pour échanger deux lignes
void Matrix2Do::swapRows(int i, int j) {
    if (i != j) {
        std::swap(data[i], data[j]);
    }
}
/**
     * @brief Transpose the matrix
     */
Matrix2Do Matrix2Do::transpose() const {
    Matrix2Do result(n, n);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            result[j][i] = data[i][j];

        }
    }
    return result;
}

// Méthode pour inverser la matrice
Matrix2Do Matrix2Do::inverse() const {
    if (n != m) {
        throw std::invalid_argument("L'inversion n'est définie que pour les matrices carrées.");
    }
    Matrix2Do augmented(*this);
    Matrix2Do result(n, true); // Matrice identité

    // Élimination de Gauss-Jordan
    for (size_t i = 0; i < n; i++) {
        // Trouver le pivot
        size_t pivotRow = i;
        for (size_t k = i + 1; k < n; k++) {
            if (std::abs(augmented[k][i]) > std::abs(augmented[pivotRow][i])) {
                pivotRow = k;
            }
        }

        // Échanger les lignes si nécessaire
        if (pivotRow != i) {
            augmented.swapRows(i, pivotRow);
            result.swapRows(i, pivotRow);
        }

        // Vérifier que le pivot n'est pas zéro
        if (std::abs(augmented[i][i]) < 1e-15) {
            std::cerr << "Erreur: matrice singulière à la ligne " << i << std::endl;
            return Matrix2Do(n, m); // Retourne une matrice vide
        }

        // Normaliser la ligne du pivot
        t_matrix pivot = augmented[i][i];
        augmented[i] /= pivot;
        result[i] /= pivot;

        // Éliminer les autres éléments de la colonne
        for (size_t k = 0; k < n; k++) {
            if (k != i) {
                t_matrix factor = augmented[k][i];

                // Soustraction ligne par ligne
                for (size_t j = 0; j < n; j++) {
                    augmented[k][j] -= factor * augmented[i][j];
                    result[k][j] -= factor * result[i][j];
                }
            }
        }
    }

    return result;
}

#pragma mark BandedMatrix

// ========== OPÉRATEURS ARITHMÉTIQUES ==========

/**
     * @brief Addition operator
     */
BandedMatrix BandedMatrix::operator+(const BandedMatrix& other) const
{
    if (m_n != other.m_n || m_m != other.m_m) {
        throw std::invalid_argument("Matrix dimensions mismatch for subtraction");
    }

    size_t maxBandWidth = std::max(m_bandWidth, other.m_bandWidth);
    BandedMatrix result(m_n, m_m, maxBandWidth);

    for (size_t i = 0; i < m_n; ++i) {
        size_t j1 = (i >= m_bandWidth) ? i - m_bandWidth : 0;
        size_t j2 = std::min(i + m_bandWidth, m_m - 1);
        size_t k1 = (i >= other.m_bandWidth) ? i - other.m_bandWidth : 0;
        size_t k2 = std::min(i + other.m_bandWidth, other.m_m - 1);

        for (size_t j = j1; j <= j2; ++j) {
            t_matrix value = 0.0L;
            if (j >= k1 && j <= k2) {
                value = at(i, j) + other.at(i, j);
            }
            result.at(i, j) = value;
        }
    }

    return result;
}

/**
     * @brief Subtraction operator
     */

BandedMatrix BandedMatrix::operator-(const BandedMatrix& other) const
{
    if (m_n != other.m_n || m_m != other.m_m) {
        throw std::invalid_argument("Matrix dimensions mismatch for subtraction");
    }

    size_t maxBandWidth = std::max(m_bandWidth, other.m_bandWidth);
    BandedMatrix result(m_n, m_m, maxBandWidth);

    for (size_t i = 0; i < m_n; ++i) {
        size_t j1 = (i >= m_bandWidth) ? i - m_bandWidth : 0;
        size_t j2 = std::min(i + m_bandWidth, m_m - 1);
        size_t k1 = (i >= other.m_bandWidth) ? i - other.m_bandWidth : 0;
        size_t k2 = std::min(i + other.m_bandWidth, other.m_m - 1);

        for (size_t j = j1; j <= j2; ++j) {
            t_matrix value = 0.0L;
            if (j >= k1 && j <= k2) {
                value = at(i, j) - other.at(i, j);
            }
            result.at(i, j) = value;
        }
    }

    return result;
}

/**
 * @brief Matrix multiplication operator
 */
BandedMatrix BandedMatrix::operator*(const BandedMatrix& other) const
{
    if (m_m != other.m_n) {
        throw std::invalid_argument("Matrix dimensions mismatch for multiplication");
    }

    size_t n = m_n;
    size_t m = other.m_m;

    // Largeur de bande du produit : au plus la somme des bandes
    size_t resultBand = std::min(n - 1, m_bandWidth + other.m_bandWidth);
    BandedMatrix result(n, m, resultBand);

    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < m; ++j) {
            if (std::abs(static_cast<int>(j) - static_cast<int>(i)) <= static_cast<int>(resultBand)) {
                t_matrix sum = 0.0L;

                size_t k_min = std::max({size_t(0),
                                         i >= m_bandWidth ? i - m_bandWidth : 0,
                                         j >= other.m_bandWidth ? j - other.m_bandWidth : 0});
                size_t k_max = std::min({m_m - 1, i + m_bandWidth, j + other.m_bandWidth});

                for (size_t k = k_min; k <= k_max; ++k) {
                    if (std::abs(static_cast<int>(k) - static_cast<int>(i)) <= static_cast<int>(m_bandWidth) &&
                        std::abs(static_cast<int>(j) - static_cast<int>(k)) <= static_cast<int>(other.m_bandWidth)) {
                        sum += this->at(i, k) * other.at(k, j);
                    }
                }

                if (std::abs(sum) > 1e-15) {
                    result.at(i, j) = sum;
                }
            }
        }
    }

    return result;
}

/**
     * @brief Scalar multiplication operator
     */

BandedMatrix BandedMatrix::operator*(t_matrix scalar) const
{
    BandedMatrix result(m_n, m_m, m_bandWidth);
    for (size_t i = 0; i < m_data.size(); ++i) {
        result.m_data[i] = m_data[i] * scalar;
    }
    return result;
}

/**
     * @brief Vector multiplication operator (Matrix × Vector)
     */
template<class U>
std::vector<t_matrix> BandedMatrix::operator*(const std::vector<U>& vec) const
{
    if (vec.size() != m_m) {
        throw std::invalid_argument("Vector size mismatch");
    }

    std::vector<t_matrix> result(m_n, 0.0L);
    for (size_t i = 0; i < m_n; ++i) {
        for (size_t j = 0; j < m_m; ++j) {
            result[i] += at(i, j) * vec[j];
        }
    }
    return result;
}


// ========== OPÉRATEURS D'ASSIGNATION ==========

/**
     * @brief Addition assignment operator
     */
BandedMatrix& BandedMatrix::operator+=(const BandedMatrix& other)
{
    if (m_n != other.m_n || m_bandWidth != other.m_bandWidth) {
        throw std::invalid_argument("Matrix dimensions or band width mismatch");
    }

    for (size_t i = 0; i < m_data.size(); ++i) {
        m_data[i] += other.m_data[i];
    }
    return *this;
}

/**
     * @brief Subtraction assignment operator
     */
BandedMatrix& BandedMatrix::operator-=(const BandedMatrix& other)
{
    if (m_n != other.m_n || m_bandWidth != other.m_bandWidth) {
        throw std::invalid_argument("Matrix dimensions or band width mismatch");
    }

    for (size_t i = 0; i < m_data.size(); ++i) {
        m_data[i] -= other.m_data[i];
    }
    return *this;
}

/**
     * @brief Scalar multiplication assignment operator
     */
BandedMatrix& BandedMatrix::operator*=(long double scalar)
{
    for (size_t i = 0; i < m_data.size(); ++i) {
        m_data[i] *= scalar;
    }
    return *this;
}

/**
     * @brief Scalar division assignment operator
     */
BandedMatrix& BandedMatrix::operator/=(long double scalar)
{
    if (abs(scalar) < 1e-15) {
        throw std::invalid_argument("Division by zero");
    }

    for (size_t i = 0; i < m_data.size(); ++i) {
        m_data[i] /= scalar;
    }
    return *this;
}

// ========== OPÉRATEURS DE COMPARAISON ==========

/**
     * @brief Equality operator
     */
bool BandedMatrix::operator==(const BandedMatrix& other) const
{
    if (m_n != other.m_n || m_bandWidth != other.m_bandWidth) {
        return false;
    }

    for (size_t i = 0; i < m_data.size(); ++i) {
        if (abs(m_data[i] - other.m_data[i]) > 1e-15) {
            return false;
        }
    }
    return true;
}

/**
     * @brief Inequality operator
     */
bool BandedMatrix::operator!=(const BandedMatrix& other) const
{
    return !(*this == other);
}

// ========== OPÉRATEURS UNAIRES ==========

/**
     * @brief Unary minus operator
     */
BandedMatrix BandedMatrix::operator-() const
{
    BandedMatrix result(m_n, m_m, m_bandWidth);
    for (size_t i = 0; i < m_data.size(); ++i) {
        result.m_data[i] = -m_data[i];
    }
    return result;
}

// ========== MÉTHODES UTILITAIRES ==========

/**
 * @brief Transpose the matrix
 */
BandedMatrix BandedMatrix::transpose() const
{
    // La transpose aura les dimensions inversées et même bande diagonale
    BandedMatrix result(m_m, m_n, m_bandWidth);

    for (size_t i = 0; i < m_n; ++i) {
        for (size_t j = std::max<int>(0, static_cast<int>(i) - static_cast<int>(m_bandWidth));
             j <= std::min(m_m - 1, i + m_bandWidth); ++j) {
            // Ne pas utiliser at(i, j) si non valide : on vérifie qu’il est bien dans la bande
            if (abs(static_cast<int>(j) - static_cast<int>(i)) <= static_cast<int>(m_bandWidth)) {
                t_matrix val = at(i, j);
                if (abs(val) > 1e-15) {
                    result.at(j, i) = val;
                }
            }
        }
    }
    return result;
}

/**
    * @brief Get the trace of the matrix
    */
t_matrix BandedMatrix::trace() const {
    if (m_n != m_m) {
        throw std::logic_error("Trace is only defined for square matrices");
    }

    t_matrix sum = 0.0L;
    for (size_t i = 0; i < m_n; ++i) {
        sum += at(i, i);
    }
    return sum;
}


/**
 * @brief Check if the matrix is symmetric
 */
bool BandedMatrix::isSymmetric() const
{
    if (m_n != m_m) {
        return false; // non carré → pas symétrique
    }

    for (size_t i = 0; i < m_n; ++i) {
        for (size_t j = i + 1; j < m_n; ++j) {
            if (abs(static_cast<int>(j) - static_cast<int>(i)) <= static_cast<int>(m_bandWidth)) {
                long double a = at(i, j);
                long double b = at(j, i);
                if (std::abs(a - b) > 1e-15) {
                    return false;
                }
            }
        }
    }
    return true;
}


/**
 * @brief Static method to create a new banded matrix
    *
    * @param n Number of rows
    * @param m Number of columns
    * @param bandWidth Number of bands on each side of the main diagonal
    * @param defaultValue Default value to initialize all elements
    * @return BandedMatrix New initialized banded matrix instance
    */
 BandedMatrix BandedMatrix::initMatrix(size_t n, size_t m, size_t bandWidth, t_matrix defaultValue ) {
    return BandedMatrix(n, m, bandWidth, defaultValue);
}


/**
    * @brief Create an identity matrix (square n × n)
    *
    * @param n Matrix dimension
    * @param bandWidth Band width (optional, default is 0)
    * @return BandedMatrix Identity matrix
    */
 BandedMatrix BandedMatrix::identity(size_t n, size_t bandWidth )
{
    BandedMatrix result(n, n, std::max(bandWidth, size_t(0)));
    for (size_t i = 0; i < n; ++i) {
        result.at(i, i) = 1.0;
    }
    return result;
}


/**
    * @brief Displays the matrix to standard output
    *
    * This method displays the complete matrix with zeros for elements
    * outside the band.
    */
void BandedMatrix::showMatrix(const std::string& str ) const
{
    std::cout << str << std::endl;
    for (size_t i = 0; i < m_n; ++i) {
        for (size_t j = 0; j < m_m; ++j) {
            if (static_cast<int>(std::abs(static_cast<int>(j) - static_cast<int>(i))) <= static_cast<int>(m_bandWidth)) {
                std::cout << std::setw(10) << std::fixed << std::setprecision(2) << at(i, j);
            } else {
                std::cout << std::setw(10) << "0.00";
            }
        }
        std::cout << std::endl;
    }
}


/**
    * @brief Create a diagonal matrix from a vector of values
    */
 BandedMatrix BandedMatrix::diagonal(const std::vector<t_matrix>& diag, size_t bandWidth)
{
    size_t n = diag.size();
    BandedMatrix result(n, n, bandWidth);
    for (size_t i = 0; i < n; ++i) {
        result.at(i, i) = diag[i];
    }
    return result;
}

// ================= DECOMPOSITION LU =====================
// NOTE: uniquement pour matrices carrées
void BandedMatrix::computeLU(BandedMatrix& L, BandedMatrix& U) const {
    if (m_n != m_m) throw std::logic_error("LU: matrix must be square");
    size_t n = m_n;
    L = BandedMatrix(n, n, m_bandWidth, 0.0);
    U = BandedMatrix(n, n, m_bandWidth, 0.0);

    for (size_t i = 0; i < n; ++i) {
        for (size_t k = i; k < std::min(n, i + m_bandWidth + 1); ++k) {
            t_matrix sum = 0;
            for (size_t j = std::max<size_t>(0, i > m_bandWidth ? i - m_bandWidth : 0); j < i; ++j)
                sum += L.at(i, j) * U.at(j, k);
            U.at(i, k) = at(i, k) - sum;
        }

        for (size_t k = i; k < std::min(n, i + m_bandWidth + 1); ++k) {
            if (i == k)
                L.at(i, i) = 1;
            else {
                t_matrix sum = 0;
                for (size_t j = std::max<size_t>(0, i > m_bandWidth ? i - m_bandWidth : 0); j < i; ++j)
                    sum += L.at(k, j) * U.at(j, i);
                if (std::abs(U.at(i, i)) < 1e-15)
                    throw std::runtime_error("LU: zero pivot encountered");
                L.at(k, i) = (at(k, i) - sum) / U.at(i, i);
            }
        }
    }
}

#pragma mark operator



#pragma mark OPÉRATEURS D'ADDITION

/**
 * @brief Addition Matrix2Do + BandedMatrix
 * @param lhs Matrice pleine
 * @param rhs Matrice bande
 * @return Matrix2Do Résultat de l'addition sous forme de matrice pleine
 */
Matrix2Do operator+(const Matrix2Do& lhs, const BandedMatrix& rhs)
{
    if (lhs.rows() != rhs.rows()|| lhs.cols() != rhs.cols()) {
        throw std::invalid_argument("Matrix2Do dimensions mismatch for addition");
    }

    Matrix2Do result(lhs.rows(), lhs.cols());

    for (size_t i = 0; i < lhs.rows(); ++i) {
        for (size_t j = 0; j < lhs.cols(); ++j) {
            t_matrix bandedValue = 0.0L;

            // Vérifier si l'élément est dans la bande
            if (std::abs(static_cast<int>(j) - static_cast<int>(i)) <=
                static_cast<int>(rhs.getBandWidth())) {
                try {
                    bandedValue = const_cast<BandedMatrix&>(rhs).at(i, j);
                } catch (const std::out_of_range&) {
                    bandedValue = 0.0L;
                }
            }

            result[i][j] = lhs[i][j] + bandedValue;
        }
    }

    return result;
}

Matrix2Do operator+(const BandedMatrix& lhs, const Matrix2Do& rhs)
{
    return rhs + lhs; // Commutativité de l'addition
}
#pragma mark OPÉRATEURS DE SOUSTRACTION

/**
 * @brief Soustraction Matrix2Do - BandedMatrix
 * @param lhs Matrice pleine
 * @param rhs Matrice bande
 * @return Matrix2Do Résultat de la soustraction sous forme de matrice pleine
 */
Matrix2Do operator-(const Matrix2Do& lhs, const BandedMatrix& rhs)
{
    if (lhs.rows() != rhs.rows()|| lhs.cols() != rhs.cols()) {
        throw std::invalid_argument("Matrix2Doo dimensions mismatch for addition");
    }

    Matrix2Do result(lhs.rows(), lhs.cols());

    for (size_t i = 0; i < lhs.rows(); ++i) {
        for (size_t j = 0; j < lhs.cols(); ++j) {
            t_matrix bandedValue = 0.0L;

            // Vérifier si l'élément est dans la bande
            if (std::abs(static_cast<int>(j) - static_cast<int>(i)) <=
                static_cast<int>(rhs.getBandWidth())) {
                try {
                    bandedValue = const_cast<BandedMatrix&>(rhs).at(i, j);
                } catch (const std::out_of_range&) {
                    bandedValue = 0.0L;
                }
            }

            result[i][j] = lhs[i][j] - bandedValue;
        }
    }

    return result;
}

/**
 * @brief Soustraction BandedMatrix - Matrix2Do
 * @param lhs Matrice bande (n × m)
 * @param rhs Matrice pleine (n × m)
 * @return Matrix2Do Résultat de la soustraction sous forme de matrice pleine (n × m)
 */
Matrix2Do operator-(const BandedMatrix& lhs, const Matrix2Do& rhs)
{
    if (rhs.rows() != lhs.rows() || rhs.cols() != lhs.cols()) {
        throw std::invalid_argument("Matrix dimensions mismatch in BandedMatrix - Matrix2Do");
    }

    size_t n = lhs.rows();
    size_t m = lhs.cols();

    Matrix2Do result (n, m);  // initialisation avec des zéros

    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < m; ++j) {
            t_matrix bandedValue = 0.0L;

            // Calcul uniquement dans la bande
            if (std::abs(static_cast<int>(j) - static_cast<int>(i)) <= static_cast<int>(lhs.getBandWidth())) {
                bandedValue = lhs.at(i, j);
            }

            result[i][j] = bandedValue - rhs[i][j];
        }
    }

    return result;
}


#pragma mark OPÉRATEURS DE MULTIPLICATION

/**
 * @brief Multiplication Matrix2Do * BandedMatrix
 * @param lhs Matrice pleine
 * @param rhs Matrice bande
 * @return Matrix2Do Résultat de la multiplication sous forme de matrice pleine
 */
Matrix2Do operator*(const Matrix2Do& lhs, const BandedMatrix& rhs)
{
    if (lhs.rows() != rhs.cols()|| lhs.cols() != rhs.rows()) {
        throw std::invalid_argument("Matrix2Do dimensions mismatch for addition");
    }
    size_t n = lhs.rows();
    size_t m = rhs.cols(); // à controler ici
    Matrix2Do result(n, m);


    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            t_matrix sum = 0.0L;

            for (size_t k = 0; k < n; ++k) {
                t_matrix bandedValue = 0.0L;

                // Vérifier si l'élément (k,j) est dans la bande de rhs
                if (std::abs(static_cast<int>(j) - static_cast<int>(k)) <=
                    static_cast<int>(rhs.getBandWidth())) {
                    try {
                        bandedValue = const_cast<BandedMatrix&>(rhs).at(k, j);
                    } catch (const std::out_of_range&) {
                        bandedValue = 0.0L;
                    }
                }

                sum += lhs[i][k] * bandedValue;
            }

            result[i][j] = sum;
        }
    }

    return result;
}

/**
 * @brief Multiplication BandedMatrix * Matrix2Do
 * @param lhs Matrice bande (n × m)
 * @param rhs Matrice pleine (m × p)
 * @return Matrix2Do Résultat de la multiplication sous forme de matrice pleine (n × p)
 */
Matrix2Do operator*(const BandedMatrix& lhs, const Matrix2Do& rhs) {
    if (lhs.cols() != rhs.rows()) {
        throw std::invalid_argument("Matrix dimensions mismatch: lhs.cols() must equal rhs.rows()");
    }

    size_t n = lhs.rows();   // nombre de lignes de la matrice bande
    size_t m = lhs.cols();   // nombre de colonnes de la matrice bande (et de lignes de rhs)
    size_t p = rhs.cols();   // nombre de colonnes du résultat

    Matrix2Do result (n, p);

    for (size_t i = 0; i < n; ++i) {
        // Parcours des colonnes dans la bande de i
        size_t k_min = (i >= lhs.getBandWidth()) ? i - lhs.getBandWidth() : 0;
        size_t k_max = std::min(i + lhs.getBandWidth(), m - 1);

        for (size_t k = k_min; k <= k_max; ++k) {
            t_matrix a = lhs.at(i, k); // dans la bande garantie
            for (size_t j = 0; j < p; ++j) {
                result[i][j] += a * rhs[k][j];
            }
        }
    }

    return result;
}



// =====================================================
// OPÉRATEURS D'ASSIGNATION AVEC OPÉRATION
// =====================================================

/**
 * @brief Addition avec assignation Matrix2Do += BandedMatrix
 * @param lhs Matrice pleine (modifiée)
 * @param rhs Matrice bande
 * @return Matrix2Do& Référence vers la matrice modifiée
 */
Matrix2Do& operator+=(Matrix2Do& lhs, const BandedMatrix& rhs)
{
    if (rhs.rows() != lhs.rows()|| rhs.cols() != lhs.cols()) {
        throw std::invalid_argument("Matrix2Do dimensions mismatch for addition");
    }

    size_t n = rhs.rows();
    size_t m = rhs.cols();

    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < m; ++j) {
            // Vérifier si l'élément est dans la bande
            if (std::abs(static_cast<int>(j) - static_cast<int>(i)) <=
                static_cast<int>(rhs.getBandWidth())) {
                try {
                    lhs[i][j] += const_cast<BandedMatrix&>(rhs).at(i, j);
                } catch (const std::out_of_range&) {
                    // L'élément est à zéro dans la matrice bande, rien à ajouter
                }
            }
        }
    }

    return lhs;
}

/**
 * @brief Soustraction avec assignation Matrix2Do -= BandedMatrix
 * @param lhs Matrice pleine (modifiée)
 * @param rhs Matrice bande
 * @return Matrix2Do& Référence vers la matrice modifiée
 */
Matrix2Do& operator-=(Matrix2Do& lhs, const BandedMatrix& rhs)
{
    if (rhs.rows() != lhs.rows()|| rhs.cols() != lhs.cols()) {
        throw std::invalid_argument("Matrix2Do dimensions mismatch for addition");
    }

    size_t n = rhs.rows();
    size_t m = rhs.cols();

    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < m; ++j) {
            // Vérifier si l'élément est dans la bande
            if (std::abs(static_cast<int>(j) - static_cast<int>(i)) <=
                static_cast<int>(rhs.getBandWidth())) {
                try {
                    lhs[i][j] -= const_cast<BandedMatrix&>(rhs).at(i, j);
                } catch (const std::out_of_range&) {
                    // L'élément est à zéro dans la matrice bande, rien à soustraire
                }
            }
        }
    }

    return lhs;
}

/**
 * @brief Multiplication avec assignation Matrix2Do *= BandedMatrix
 * @param lhs Matrice pleine (modifiée)
 * @param rhs Matrice bande
 * @return Matrix2Do& Référence vers la matrice modifiée
 */
Matrix2Do& operator*=(Matrix2Do& lhs, const BandedMatrix& rhs)
{
    lhs = lhs * rhs; // Utilise l'opérateur * déjà défini
    return lhs;
}
// =====================================================
// FONCTIONS UTILITAIRES
// =====================================================

/**
 * @brief Convertit une BandedMatrix en Matrix2Do
 * @param banded Matrice bande à convertir
 * @return Matrix2Do Matrice pleine équivalente
 */
Matrix2Do toMatrix2Do(const BandedMatrix& banded)
{
    Matrix2Do result(banded.rows(), banded.cols());

    for (size_t i = 0; i < banded.rows(); ++i) {
        for (size_t j = 0; j < banded.cols(); ++j) {
            if (std::abs(static_cast<int>(j) - static_cast<int>(i)) <=
                static_cast<int>(banded.getBandWidth())) {
                try {
                    result[i][j] = const_cast<BandedMatrix&>(banded).at(i, j);
                } catch (const std::out_of_range&) {
                    result[i][j] = 0.0L;
                }
            } else {
                result[i][j] = 0.0L;
            }
        }
    }

    return result;
}

/**
 * @brief Convertit une Matrix2Do en BandedMatrix (si possible)
 * @param matrix Matrice pleine à convertir
 * @param bandWidth Largeur de bande désirée
 * @return BandedMatrix Matrice bande équivalente
 * @throws std::invalid_argument Si la matrice ne peut pas être représentée avec la largeur de bande donnée
 */
BandedMatrix toBandedMatrix(const Matrix2Do& matrix, size_t bandWidth)
{
    BandedMatrix result(matrix.rows(), matrix.cols(), bandWidth);

    // Vérifier que tous les éléments hors bande sont nuls (ou très petits)
    const t_matrix tolerance = 1e-12L;

    for (size_t i = 0; i < matrix.rows(); ++i) {
        for (size_t j = 0; j < matrix.cols(); ++j) {
            if (std::abs(static_cast<int>(j) - static_cast<int>(i)) <=
                static_cast<int>(bandWidth)) {
                result.at(i, j) = matrix[i][j];
            } else if (std::abs(matrix[i][j]) > tolerance) {
                throw std::invalid_argument("Matrix2Do cannot be represented as banded matrix with given bandwidth");
            }
        }
    }

    return result;
}


/** Write Data
 */
QDataStream &operator<<( QDataStream &stream , const CurveMap &map )
{
    stream << map._row;
    stream << map._column;
    stream << map.min_value;
    stream << map.max_value;
    stream << map.rangeX;
    stream << map.rangeY;

   // stream << (quint32) map.data.size();
    //for (const double* v = begin(map.data); v != end(map.data); ++v)
     //   stream << *v;
    for (const double v : map.data)
        stream << v;

   return stream;
}

/** Read Data
 */
QDataStream &operator>>( QDataStream &stream, CurveMap &map )
{
    stream >> map._row;
    stream >> map._column;
    stream >> map.min_value;
    stream >> map.max_value;
    stream >> map.rangeX;
    stream >> map.rangeY;

    unsigned long siz = map._column * map._row;

    map.data.resize(siz);// = std::valarray<double>(siz);


    double v;
    for (unsigned long i = 0; i < siz; ++i) {
        stream >> v;
        map.data[i] = v ;
    }

    return stream;

}

void showMatrix(const Matrix2D& m, const std::string& str)
{
    std::cout << str << "\n";
    std::cout << m << std::endl;
}

void showMatrix(const DiagonalMatrixLD& d, const std::string& str)
{
    std::cout << str << "\n";
    std::cout << d.toDenseMatrix() << std::endl;
}


void showVector(const std::vector<t_matrix> &m, const std::string& str)
{
 std::cout << str << "\n";
 for(unsigned long i = 0; i < m.size(); i++) {
     if (std::is_same<t_matrix, long double>::value == true) {
         printf(" %20.65Lf ", m[i]);
     }
     else if (std::is_same<t_matrix, double>::value == true) {
         printf(" %8.65f ",(double) m[i]);
     }

   printf("\n");
 }
 printf("\n");
}

void showVector(const std::vector<double> &m, const std::string& str)
{
    std::cout << str << "\n";
    for(unsigned long i = 0; i < m.size(); i++) {
        //printf(" %8.65f ",(double) m[i]);
        printf(" %8.5f ",(double) m[i]);
        printf("\n");
    }
    printf("\n");
}

