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

#include "CurveUtilities.h"
#include "Model.h"
#include "StdUtilities.h"
#include "eigen_3.4.0/Eigen/src/Core/DiagonalMatrix.h"

#ifdef DEBUG
#ifdef Q_OS_MAC
   #include <cfenv>
#endif
#endif

#include <eigen_3.4.0/Eigen/Dense>
#include <eigen_3.4.0/Eigen/OrderingMethods>

#include <algorithm>
#include <iostream>

extern QString res_file_version;

/**
 * @brief Calculates the vector of differences between consecutive events' reduced theta values.
 *
 * This function computes the difference between each pair of consecutive events in the input vector,
 * specifically subtracting the mThetaReduced value of each event from that of the next event.
 *
 * @param events Vector of shared pointers to Event objects
 * @return std::vector<t_reduceTime> Vector containing the differences between consecutive events'
 *                                  mThetaReduced values
 *
 * @note The returned vector size is one less than the input vector size.
 * @note When compiled with DEBUG defined, the function will output debug information for differences
 *       that are less than or equal to 1.0E-10.
 * @throw std::invalid_argument If the input vector contains fewer than 2 events
 */
std::vector<t_reduceTime> calculVecH(const std::vector<std::shared_ptr<Event>>& events)
{
    // Verification for empty or single-element vector
#ifdef DEBUG
    const size_t size = events.size();
    if (size < 2) {
        throw std::invalid_argument("calculVecH requires at least 2 events");
    }
#endif

    // Reserve memory for result vector (more efficient than default constructor)
    std::vector<t_reduceTime> result;
    result.reserve(events.size() - 1);

    // Define lambda for the difference calculation
    auto diffX = [](const std::shared_ptr<Event>& e0, const std::shared_ptr<Event>& e1) -> t_reduceTime {
        return e1->mThetaReduced - e0->mThetaReduced;
    };

    // Transform adjacent elements using the difference lambda
    std::transform(events.begin(), events.end() - 1, events.begin() + 1, std::back_inserter(result), diffX);

#ifdef DEBUG
    // Debug output for very small differences
    for (size_t i = 0; i < result.size(); ++i) {
        const t_reduceTime diff = result[i];
        if (diff <= 1.0E-10) {
            qDebug() << "[CurveUtilities::calculVecH] diff Theta r <= 1.0E-10 "
                     << static_cast<double>(events[i]->mThetaReduced) << " "
                     << static_cast<double>(events[i + 1]->mThetaReduced);
        }
    }
#endif

    return result;
}


// --------------- Function with list of double value
/**
 * @brief Calculates the R0 matrix for cubic spline interpolation without zero padding.
 *
 * This function computes the full R0 matrix used in cubic spline calculations.
 * Unlike calculMatR, this returns the dense matrix without zero padding.
 * The matrix is symmetric with values based on the differences in the vec_h input.
 *
 * @param vec_h Vector of time differences between consecutive points (size n+1)
 * @return Matrix2D Dense n×n matrix R0 for cubic spline calculations
 *
 * @note Matrix entries are calculated as follows:
 *       - Diagonal: (h_i + h_{i+1})/3
 *       - Off-diagonal (i,i+1) and (i+1,i): h_{i+1}/6
 * @throw std::invalid_argument If vec_h has fewer than 2 elements
 */
 // to be used only with Komlan
Matrix2D calculMatR0(const std::vector<t_reduceTime>& vec_h)
{
    // Check input size
#ifdef DEBUG
    if (vec_h.size() < 2) {
        throw std::invalid_argument("calculMatR0 requires at least 2 elements in vec_h");
    }
#endif

    // Matrix size is one less than vec_h size
    const size_t n = vec_h.size() - 1;

    // Initialize matrix with zeros
    Matrix2D matR0 (n, n);

    // Pre-compute constants to avoid repeated divisions
    constexpr t_matrix ONE_THIRD = 1.0L/3.0L;
    constexpr t_matrix ONE_SIXTH = 1.0L/6.0L;

    // Fill the matrix
    for (size_t i = 0; i < n - 1; ++i) {
        // Diagonal elements
        matR0(i, i) = (vec_h[i] + vec_h[i+1]) * ONE_THIRD;

        // Symmetric off-diagonal elements (calculate once, assign twice)
        t_matrix offDiagonal = vec_h[i+1] * ONE_SIXTH;
        matR0(i, i+1) = offDiagonal;
        matR0(i+1, i) = offDiagonal;  // Symmetric element
    }

    // Last diagonal element (special case)
    matR0(n-1, n-1) = (vec_h[n-1] + vec_h[n]) * ONE_THIRD;

    return matR0;
}

/**
 * @brief Calculates the R matrix contained within an n x n matrix.
 *
 * This function computes the R matrix of size (n-2) x (n-2), placed in the center of an n x n matrix.
 * The R matrix is constructed using values from the input vector vec_h.
 * For example, when n=5:
 *   0 0 0 0 0
 *   0 X X X 0
 *   0 X X X 0
 *   0 X X X 0
 *   0 0 0 0 0
 *
 * @param vec_h The input vector of size n-1, where n is the size of the containing matrix.
 * @return Matrix2D The computed R matrix within an n x n matrix.
 */
/*Matrix2D calculMatR(const std::vector<t_reduceTime> &vec_h)
{
    // vecH est de dimension n-1
    const unsigned long n = vec_h.size() + 1;

    Matrix2D matR = Matrix2D::Zero(n, n);

    // Pre-compute constants to avoid repeated divisions
    constexpr t_matrix ONE_THIRD = 1.0L/3.0L;
    constexpr t_matrix ONE_SIXTH = 1.0L/6.0L;

    for ( unsigned long i = 1; i < n-2; ++i) {
        matR(i, i) = (vec_h[i-1] + vec_h[i]) * ONE_THIRD;
        t_matrix offDiagonal = vec_h[i] * ONE_SIXTH;
        matR(i, i+1) = offDiagonal;
        matR(i+1, i) = offDiagonal;
    }
    // Si on est en n-2 (dernière itération), on ne calcule pas les valeurs de part et d'autre de la diagonale (termes symétriques)
    matR(n-2, n-2) = (vec_h[n-2-1] + vec_h[n-2]) * ONE_THIRD;

    return matR;
}
*/

/**
 * @brief Calculates the R matrix as a sparse matrix contained within an n x n matrix.
 *
 * This function computes the R matrix of size (n-2) x (n-2), placed in the center of an n x n matrix
 * using Eigen's SparseMatrix for efficient storage of the mostly-zero matrix.
 * The R matrix is constructed using values from the input vector vec_h.
 * For example, when n=5:
 *   0 0 0 0 0
 *   0 X X X 0
 *   0 X X X 0
 *   0 X X X 0
 *   0 0 0 0 0
 *
 * @param vec_h The input vector of size n-1, where n is the size of the containing matrix.
 * @return Eigen::SparseMatrix<t_matrix> The computed sparse R matrix within an n x n matrix.
 */
SparseMatrixLD calculMatR(const std::vector<t_reduceTime> &vec_h)
{
    // vecH est de dimension n-1
    const unsigned long n = vec_h.size() + 1;

    // Créer la matrice creuse
    Eigen::SparseMatrix<t_matrix> matR(n, n);

    // Pre-compute constants to avoid repeated divisions
    constexpr t_matrix ONE_THIRD = 1.0L/3.0L;
    constexpr t_matrix ONE_SIXTH = 1.0L/6.0L;

    // Estimer le nombre d'éléments non-nuls pour optimiser l'allocation
    // Pour une matrice tridiagonale de taille (n-2)x(n-2), on a:
    // - (n-2) éléments diagonaux
    // - 2*(n-3) éléments hors-diagonale
    const unsigned long nnz_estimate = (n-2) + 2*(n-3);
    matR.reserve(nnz_estimate);

    // Utiliser un vecteur de triplets pour construire efficacement la matrice
    std::vector<Eigen::Triplet<t_matrix>> triplets;
    triplets.reserve(nnz_estimate);

    // Remplir les triplets avec les valeurs non-nulles
    for (unsigned long i = 1; i < n-2; ++i) {
        // Élément diagonal
        t_matrix diagonal = (vec_h[i-1] + vec_h[i]) * ONE_THIRD;
        triplets.emplace_back(i, i, diagonal);

        // Éléments hors-diagonale (symétriques)
        t_matrix offDiagonal = vec_h[i] * ONE_SIXTH;
        triplets.emplace_back(i, i+1, offDiagonal);
        triplets.emplace_back(i+1, i, offDiagonal);
    }

    // Dernière itération (i = n-2)
    t_matrix lastDiagonal = (vec_h[n-3] + vec_h[n-2]) * ONE_THIRD;
    triplets.emplace_back(n-2, n-2, lastDiagonal);

    // Construire la matrice à partir des triplets
    matR.setFromTriplets(triplets.begin(), triplets.end());

    // Compresser la matrice pour optimiser les opérations ultérieures
    matR.makeCompressed();

    return matR;
}


/**
 * @brief Computes the Q matrix of size n x (n-2) based on the provided vector vec_h.
 *
 * The Q0 matrix has a banded structure with non-zero elements on the main diagonal and the first off-diagonal.
 * The matrix is constructed such that:
 * - The main diagonal elements are the negative sum of consecutive reciprocals of vec_h.
 * - The first off-diagonal elements are the reciprocals of vec_h.
 *
 * For example, when n=5:
 * +X   0   0
 * -X  +a   0
 * +a  -X  +b
 * +0  +b  -X
 *  0   0  +X
 * @param vec_h The input vector of size n-1 containing positive values.
 * @return Matrix2D The resulting Q0 matrix of dimensions n x (n-2).
 * Plus rapide que calculMat0(), car pré-calcul des inverses
 */
Matrix2D calculMatQ00(const std::vector<t_reduceTime>& vec_h)
{
    const size_t n = vec_h.size() + 1;
#ifdef DEBUG
    // Vérification de la taille minimale de vec_h
    if (vec_h.size() < 2) {
        throw std::invalid_argument("vec_h doit avoir au moins deux éléments.");
    }
#endif
    // matQ0 est de dimension n x (n-2)
    Matrix2D matQ0 = Matrix2D::Zero (n, n - 2);

    // Pré-calcul des inverses pour éviter les divisions répétées
    std::vector<t_matrix> inv_h(n - 1);
    for (size_t i = 0; i < n - 1; ++i) {
#ifdef DEBUG
        if (vec_h[i] <= 0) {
            throw std::runtime_error("[CurveUtilities::calculMatQ00] vec_h <= 0");
        }
#endif
        inv_h[i] = 1.0L / vec_h[i];

    }

    // Remplissage de la matrice
    for (size_t i = 1; i < n - 1; ++i) {
        matQ0(i - 1, i - 1) = inv_h[i - 1];
        matQ0(i, i - 1) = -(inv_h[i - 1] + inv_h[i]);
        matQ0(i + 1, i - 1) = inv_h[i];
    }

    return matQ0;

}

//la matrice Q est une matrice de bande 3, de dimension n x (n-2)
// to be used only with Komlan
Matrix2D calculMatQ0(const std::vector<t_reduceTime>& vec_h)
{
    // Calcul de la matrice Q, de dimension n x (n-2)
    // Par exemple pour n = 5 :
    // X 0 0
    // X X X
    // X X X
    // X X X
    // 0 0 X

    // vec_h est de dimension n-1
    const size_t n = vec_h.size() + 1;

    // matQ0 est de dimension n x (n-2)
    Matrix2D matQ = Matrix2D::Zero (n, n - 2);

    // On parcourt n-2 valeurs :
    for (size_t i = 1; i < n - 1; ++i) {
#ifdef DEBUG
        if (vec_h.at(i - 1) <= 0) {
            throw std::runtime_error("[CurveUtilities::calculMatQ0] vec_h <= 0");
        }
#endif
        matQ(i - 1, i - 1) = 1.0L / vec_h[i - 1];
        matQ(i, i - 1) = -((1.0L / vec_h[i - 1]) + (1.0L / vec_h[i]));
        if (i < n - 1) {
            matQ(i + 1, i - 1) = 1.0L / vec_h[i];
        }


    }


    // Par exemple pour n = 5 :
    // +X  0  0
    // -X +a  0
    // +a -X +b
    //  0 +b -X
    //  0  0 +X

    return matQ;
}


// Dans RenCurve procedure Calcul_Mat_Q_Qt_R ligne 55
/*
Matrix2D calculMatQ(const std::vector<t_reduceTime>& vec_h)
{
    // Calcul de la matrice Q, de dimension n x (n-2) contenue dans une matrice n x n
    // Les 1ère et dernière colonnes sont nulles
    // Par exemple pour n = 5 :
    // 0 X 0 0 0
    // 0 X X X 0
    // 0 X X X 0
    // 0 X X X 0
    // 0 0 0 X 0

    // vecH est de dimension n-1
    const size_t n = vec_h.size() + 1;

    // matQ est de dimension n x n-2, mais contenue dans une matrice nxn
    Matrix2D matQ = Matrix2D::Zero (n, n);
    // On parcourt n-2 valeurs :
    for (size_t i = 1; i < n-1; ++i) {
        matQ(i-1, i) = 1.0L  / vec_h[i-1];
        matQ(i, i) = -((1.0L / vec_h[i-1]) + (1.0L /vec_h[i]));
        matQ(i+1, i) = 1.0L  / vec_h[i];


    }
    // pHd : ici la vrai forme est une matrice de dimension n x (n-2), de bande k=1; les termes diagonaux sont négatifs
    // Les 1ère et dernière colonnes sont nulles
    // Par exemple pour n = 5 :
    // 0 +X  0  0 0
    // 0 -X +a  0 0
    // 0 +a -X +b 0
    // 0  0 +b -X 0
    // 0  0  0 +X 0

    return matQ;
}
*/

/**
 * @brief Calculates the Q matrix as a sparse matrix of dimension n x (n-2) contained within an n x n matrix.
 *
 * This function computes the Q matrix using Eigen's SparseMatrix for efficient storage.
 * The first and last columns are null (zero).
 * For example, when n=5:
 * The actual structure is a banded matrix with k=1; diagonal terms are negative:
 *   0 +X  0  0 0
 *   0 -X +a  0 0
 *   0 +a -X +b 0
 *   0  0 +b -X 0
 *   0  0  0 +X 0
 *
 * @param vec_h The input vector of size n-1, where n is the size of the containing matrix.
 * @return Eigen::SparseMatrix<t_matrix> The computed sparse Q matrix within an n x n matrix.
 * @throws std::runtime_error if any element in vec_h is <= 0 (in DEBUG mode).
 */
SparseMatrixLD calculMatQ(const std::vector<t_reduceTime>& vec_h)
{
    // vecH est de dimension n-1
    const size_t n = vec_h.size() + 1;

    // Créer la matrice creuse n x n
    Eigen::SparseMatrix<t_matrix> matQ(n, n);

    // Estimer le nombre d'éléments non-nuls
    // Pour chaque colonne i (i = 1 à n-2), on a au maximum 3 éléments non-nuls
    // Mais les colonnes aux extrémités peuvent avoir moins d'éléments
    // Estimation conservative: 3 * (n-2)
    const size_t nnz_estimate = 3 * (n - 2);
    matQ.reserve(nnz_estimate);

    // Utiliser un vecteur de triplets pour construire efficacement la matrice
    std::vector<Eigen::Triplet<t_matrix>> triplets;
    triplets.reserve(nnz_estimate);

    // Parcourir les n-2 colonnes (i = 1 à n-2)
    for (size_t i = 1; i < n-1; ++i) {
#ifdef DEBUG
        if (vec_h[i-1] <= 0 || vec_h[i] <= 0)
            throw std::runtime_error("[CurveUtilities::calculMatQ] vec_h <= 0");
#endif

        // Précalculer les inverses pour éviter les divisions répétées
        const t_matrix inv_h_prev = 1.0L / vec_h[i-1];
        const t_matrix inv_h_curr = 1.0L / vec_h[i];

        // Élément au-dessus de la diagonale: matQ(i-1, i)
        triplets.emplace_back(i-1, i, inv_h_prev);

        // Élément diagonal: matQ(i, i) - négatif
        const t_matrix diagonal = -(inv_h_prev + inv_h_curr);
        triplets.emplace_back(i, i, diagonal);

        // Élément en-dessous de la diagonale: matQ(i+1, i)
        triplets.emplace_back(i+1, i, inv_h_curr);
    }

    // Construire la matrice à partir des triplets
    matQ.setFromTriplets(triplets.begin(), triplets.end());

    // Compresser la matrice pour optimiser les opérations ultérieures
    matQ.makeCompressed();

    return matQ;
}

/**
 * @brief Calcule A = I + (-lambda) * W_1 * Q * B_1 * Q^T
 *
 * Toutes les opérations sont déroulées manuellement avec des boucles.
 *
 * @param Q Matrice Q (n x k)
 * @param B1 Matrice B1 (k x k)
 * @param W1 Matrice diagonale W1 (n x n), représentée ici comme vecteur diagonal
 * @param lambda Le scalaire lambda
 * @return Matrix2D La matrice A (n x n)
 */
Matrix2D computeMatA_direct(const Matrix2D& Q, const Matrix2D& B1, const DiagonalMatrixLD& W1_diag, double lambda)
{
    size_t n = Q.rows();        // lignes de Q
    size_t k = Q.cols();    // colonnes de Q = taille de B1

    // Initialiser A à la matrice identité
    Matrix2D A = Matrix2D::Zero (n, k);
   // for (size_t i = 0; i < n; ++i)
     //   A[i] (0.0, n);

    // Étapes :
    // A = I + (-lambda) * W1_diag[i] * (Q * B1 * Q^T)[i][j]
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            t_matrix sum = 0.0;
            for (size_t u = 0; u < k; ++u) {
                for (size_t v = 0; v < k; ++v) {
                    sum += Q(i, u) * B1(u, v) * Q(j, v);
                }
            }
            t_matrix val = -lambda * W1_diag.diagonal()[i] * sum;
            A(i, j) = (i == j ? 1.0 : 0.0) + val;
        }
    }

    return A;
}

/**
 * @brief Calcule A = I + (-lambda) * diag(W_1) * Q * B_1 * Q^T
 *
 * Optimisé avec :
 * - Remaniement d’ordre des calculs
 * - Sommation de Kahan pour précision
 *
 * @param Q Matrice (n x k)
 * @param B_1 Matrice  (k x k)
 * @param W_1 Diagonale de W_1 (vecteur de taille n)
 * @param lambda Scalaire
 * @return Matrix2D Matrice A (n x n)
 *
 * Raisons pour ne pas utiliser Cholesky dans le code actuel :

    Tu ne cherches pas à résoudre un système linéaire :

        Cholesky est surtout utile pour résoudre Ax =b.

        Ici tu construis explicitement la matrice A, tu ne cherches pas à résoudre un système.

    A n’est pas toujours SPD :

        La matrice A peut ne pas être définie positive, car elle dépend de λ, B_1​, et W_1​.

        Même si B_1​ est SPD, le produit peut introduire des zéros, ou rendre A semi-définie.

    Tu n’as pas besoin de factoriser A, juste de l’évaluer explicitement.
 */
Matrix2D computeMatA_optimized_kahan(const Matrix2D& Q,
                                     const Matrix2D& B_1,
                                     const DiagonalMatrixLD &W_1,
                                     double lambda)
{
    /*
       const Matrix2D& QB_1QT = multiMatParMat0(Q, multiMatParMat0(B_1, QT));

        const Matrix2D& W_1QB_1QT = multiDiagParMat0(W_1, QB_1QT);

        const Matrix2D& lambdaW_1QB_1QT = multiConstParMat0(W_1QB_1QT, - mModel->mLambdaSpline.mX);

        const Matrix2D& A = addIdentityToMat(lambdaW_1QB_1QT);
    */

    size_t n = Q.rows();        // nombre de lignes
    size_t k = Q.cols();     // nombre de colonnes

    // Étape 1 : M = Q * B1 (taille n x k)
    Matrix2D M = Matrix2D::Zero (n, k);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < k; ++j) {
            t_matrix sum = 0.0, c = 0.0;
            for (size_t l = 0; l < k; ++l) {
                t_matrix prod = Q(i, l) * B_1(l, j);
                t_matrix y = prod - c;
                t_matrix t = sum + y;
                c = (t - sum) - y;
                sum = t;
            }
            M(i, j) = sum;
        }
    }

    // Étape 2 : T = M * Q^T → taille n x n
    Matrix2D T = Matrix2D::Zero (n, n);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < k; ++j) {
            t_matrix sum = 0.0, c = 0.0;
            for (size_t l = 0; l < k; ++l) {
                t_matrix prod = M(i, l) * Q(j, l);  // Q^T[j][l] == Q[j][l]
                t_matrix y = prod - c;
                t_matrix t = sum + y;
                c = (t - sum) - y;
                sum = t;
            }
            T(i, j) = sum;
        }
    }

    // Étape 3 : A = I + (-lambda) * diag(W1) * T
    Matrix2D A = Matrix2D::Zero (n, n);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            t_matrix scaled = -lambda * W_1.diagonal()[i] * T(i, j);
            A (i, j) = (i == j ? 1.0 : 0.0) + scaled;
        }
    }

    return A;
}


/**
 * @brief Calcule B_1 = (R + lambda * Q^T * W1 * Q)^(-1) en une seule fonction
 *
 * Cette version :
 * - Calcule directement Q^T * (W1 * Q) sans passer par des fonctions intermédiaires
 * - Évite les allocations inutiles
 * - Utilise l'inversion directe ou via Cholesky selon la taille
 *
 * @param Q         Matrice Q (n x k)
 * @param QT        Transposée de Q (k x n)
 * @param W_1       Diagonale de W_1 (valarray de taille n)
 * @param R         Matrice R (k x k)
 * @param lambda    Scalaire lambda
 * @return Matrix2D B_1 (k x k)
 */

// Problème R est une matrice padded donc l'inversion est possible seulement au centre
Matrix2D computeB_1_from_Q_W1_R_direct(const Matrix2D& Q, const DiagonalMatrixLD &W_1, const Matrix2D& R, double lambda)
{
    /*auto QT = transpose0(Q);
    const Matrix2D& QTW_1Q_test = multiMatParMat0(QT, multiDiagParMat0(W_1, Q));

    const Matrix2D& lambdaQTW_1Q = multiConstParMat0(QTW_1Q_test, lambda);
    //showMatrix(lambdaQTW_1Q, "lambdaQTW_1Q");
    const Matrix2D& B_test = addMatEtMat0(R, lambdaQTW_1Q); // Correspond à notre matB
    showMatrix(B_test, "B_test computeB_1_from_Q_W1_R_direct") ;
    const auto& decomp_test = cholesky_LDLt_MoreSorensen(B_test);

    auto B_1_test = choleskyInvert(decomp_test);

    showMatrix(B_1_test, "[computeB_1_from_Q_W1_R_direct] B_1_test ") ;
    */

    const size_t n = Q.rows();        // Nombre de lignes
    const size_t k = Q.cols();     // Nombre de colonnes

    // Calcul de Q^T * (W_1 * Q)
    Matrix2D QTW_1Q = Matrix2D::Zero (k, k);

    for (size_t l = 0; l < n; ++l) {
        t_matrix w = W_1.diagonal()[l]; // coefficient de la diagonale
        for (size_t i = 0; i < k; ++i) {
            for (size_t j = 0; j < k; ++j) {
                QTW_1Q (i, j) += Q(l, i) * w * Q(l, j) ; // Q^T * diag(W_1) * Q où Q^T[i][l] = Q[l][i]
            }
        }
    }

    // B = R + lambda * QTW_1Q
    Matrix2D B = Matrix2D::Zero (k, k);
    for (size_t i = 0; i < k; ++i) {
        for (size_t j = 0; j < k; ++j) {
            B(i, j) = R(i, j) + lambda * QTW_1Q(i, j);
        }
    }

    // Décomposition LU, plus stable et plus rapide
    Eigen::PartialPivLU<Matrix2D> lu(B);

    // Inverser la matrice
    Matrix2D inverseMatrix = lu.inverse();


    // Inversion de B
    /*Matrix2D B_1 = Matrix2D::Zero  (k, k);
    if (k <= 3) {
        B_1 = inverseMatSym0(B, 0);  // Inversion explicite pour petites matrices

    } else {
        const auto& decomp = cholesky_LDLt_MoreSorensen(B);
        B_1 = choleskyInvert(decomp);
    }
    showMatrix(B_1, " B_1");

    Matrix2D B_try2 = (R +  lambda * Q.transpose() *  W_1 * Q);
    Matrix2D B_1_try2 = B_try2.inverse();
    showMatrix(B_1_try2, " B_1_try2");

    return B_1;*/

    return inverseMatrix;
}


Matrix2D compute_AxBxAT(const Matrix2D& A, const Matrix2D& B)
{
    Matrix2D M = multiMatParMat0(A, B);             // A * B  => (m x k)
    Matrix2D AT = transpose0(A);            // A^T    => (k x m)
    Matrix2D C = multiMatParMat0(M, AT);            // M * A^T => (m x m)
    return C;

}


#pragma mark Init vectors et matrix


// can be replace with #include <functional>   // std::greater
// unused
bool sortItems(const double& a, const double& b)
{
    return (a < b);
}

bool sortItems(const t_matrix& a, const t_matrix& b)
{
    return (a < b);
}

void display(const std::vector<double>& v)
{
    for (std::vector<double>::const_iterator it=v.begin(); it!=v.end(); ++it) {
        std::cout << *it << ' ' ;
    }
    std::cout << std::endl;
}


#include <cmath>

/**
 * @brief Convertit les coordonnées polaires (Inclinaison, Déclinaison, Intensité) en un vecteur 3D (x, y, z).
 *
 * Données d'entrée : un vecteur en coordonnées polaires \f$ (Inc, Dec, F) \f$.
 *
 * Formules :
 * - \f$ z = F \cdot \sin(\text{Inc}) \f$
 * - \f$ F_{xy} = F \cdot \cos(\text{Inc}) \f$
 * - \f$ x = F_{xy} \cdot \cos(\text{Dec}) \f$
 * - \f$ y = F_{xy} \cdot \sin(\text{Dec}) \f$
 *
 * Inclinaison (Inc) et déclinaison (Dec) sont attendues en **degrés**.
 *
 * @param Inc inclinaison en degrés (angle entre F et l'horizontale)
 * @param Dec déclinaison en degrés (angle entre le nord géographique et la projection horizontale)
 * @param F intensité totale (norme du vecteur)
 * @param[out] x composante x du vecteur
 * @param[out] y composante y du vecteur
 * @param[out] z composante z du vecteur
 */
void convertToXYZ(double Inc, double Dec, double F, double &x, double &y, double &z)
{
    // Convertir les angles de degrés à radians
    double Inc_rad = Inc * M_PI / 180.0;
    double Dec_rad = Dec * M_PI / 180.0;

    // Calculer les coordonnées cartésiennes
    z = F * std::sin(Inc_rad);
    double F_xy = F * std::cos(Inc_rad);
    x = F_xy * std::cos(Dec_rad);
    y = F_xy * std::sin(Dec_rad);
}

/**
 * @brief Convertit un vecteur 3D (x, y, z) en Inclinaison, Déclinaison et Intensité.
 *
 * Données d'entrée : un vecteur \f$ \vec{g} = (x, y, z) \f$ dans un repère géocentrique.
 *
 * Formules :
 * - \f$ F = \sqrt{x^2 + y^2 + z^2} \f$
 * - \f$ \text{Inc} = \arcsin \left( \frac{z}{F} \right) \f$
 * - \f$ \text{Dec} = \arctan2(y, x) \f$
 *
 * Inclinaison (Inc) et déclinaison (Dec) sont retournées en **degrés**.
 *
 * @param x composante x du vecteur
 * @param y composante y du vecteur
 * @param z composante z du vecteur
 * @param[out] F intensité totale (norme du vecteur)
 * @param[out] Inc inclinaison en degrés (angle entre F et l'horizontale)
 * @param[out] Dec déclinaison en degrés (angle entre le nord géographique et la projection horizontale)
 */
void convertToIDF(double x, double y, double z, double &Inc, double& Dec, double& F)
{
    F = std::sqrt(x * x + y * y + z * z);

    if (F == 0.0) {
        Inc = 0.0;
        Dec = 0.0;
        return;
    }

    Inc = std::asin(z / F) * 180.0 * M_1_PI;        // en degrés
    Dec = std::atan2(y, x) * 180.0 * M_1_PI;        // en degrés
}

/**
 * @brief Calcule les dérivées premières de F, Inclinaison (Inc) et Déclinaison (Dec).
 *
 * Données :
 * - \f$ \vec{g} = (gx, gy, gz) \f$ : composantes du champ magnétique
 * - \f$ \vec{g}' = (gpx, gpy, gpz) \f$ : dérivées premières
 *
 * Formules :
 * - \f$ F = \sqrt{gx^2 + gy^2 + gz^2} \f$
 * - \f$ \frac{dF}{dt} = \frac{gx \cdot gpx + gy \cdot gpy + gz \cdot gpz}{F} \f$
 *
 * - \f$ \text{Inc} = \arcsin \left( \frac{gz}{F} \right) \Rightarrow u = \frac{gz}{F} \f$
 * - \f$ \frac{d \text{Inc}}{dt} = \frac{1}{\sqrt{1 - u^2}} \cdot \frac{gpz \cdot F - gz \cdot \frac{dF}{dt}}{F^2} \f$
 *
 * - \f$ \text{Dec} = \arctan2(gy, gx) \f$
 * - \f$ \frac{d \text{Dec}}{dt} = \frac{gx \cdot gpy - gy \cdot gpx}{gx^2 + gy^2} \f$
 *
 * @param gx composante x du champ
 * @param gy composante y du champ
 * @param gz composante z du champ
 * @param gpx dérivée de gx
 * @param gpy dérivée de gy
 * @param gpz dérivée de gz
 * @param[out] dFdt dérivée de F (en nT/s)
 * @param[out] dIncdt dérivée de l'inclinaison (en degrés/s)
 * @param[out] dDecdt dérivée de la déclinaison (en degrés/s)
 */
void computeDerivatives(double gx, double gy, double gz,
                        double gpx, double gpy, double gpz,
                        double& dIncdt, double& dDecdt, double& dFdt)
{
    // Calcul de F
    double F = std::sqrt(gx * gx + gy * gy + gz * gz);

    // Sécurité contre division par zéro
    if (F == 0.0) {
        dFdt = dIncdt = dDecdt = 0.0;
        return;
    }

    // dF/dt
    dFdt = (gx * gpx + gy * gpy + gz * gpz) / F;

    // dInc/dt
    double denomInc = std::sqrt(F * F - gz * gz);
    if (denomInc == 0.0) {
        dIncdt = 0.0; // Peut être ajusté selon le cas limite

    } else {
        dIncdt = (gpz - (gz / F) * dFdt) / denomInc;
    }

    // dDec/dt
    double denomDec = gx * gx + gy * gy;
    if (denomDec == 0.0) {
        dDecdt = 0.0; // Peut être ajusté selon le cas limite

    } else {
        dDecdt = (gx * gpy - gy * gpx) / denomDec;
    }

    // Conversion en degrés
    dIncdt *= 180.0 * M_1_PI;
    dDecdt *= 180.0 * M_1_PI;
}

/**
 * @brief Calcule les dérivées secondes de F, Inclinaison (Inc) et Déclinaison (Dec).
 *
 * Données :
 * - \f$ \vec{g} = (gx, gy, gz) \f$
 * - \f$ \vec{g}' = (gpx, gpy, gpz) \f$ : dérivées premières
 * - \f$ \vec{\gamma} = (\gamma_x, \gamma_y, \gamma_z) \f$ : dérivées secondes
 *
 * Formules :
 * - \f$ F = \sqrt{gx^2 + gy^2 + gz^2} \f$
 * - \f$ \frac{dF}{dt} = \frac{gx \cdot gpx + gy \cdot gpy + gz \cdot gpz}{F} \f$
 * - \f$ \frac{d^2F}{dt^2} = \frac{gpx^2 + gpy^2 + gpz^2 + gx \cdot \gamma_x + gy \cdot \gamma_y + gz \cdot \gamma_z}{F} - \left( \frac{dF}{dt} \right)^2 \cdot \frac{1}{F} \f$
 *
 * - \f$ \text{Inc} = \arcsin \left( \frac{gz}{F} \right) \Rightarrow u = \frac{gz}{F} \f$
 * - \f$ \frac{d^2 \text{Inc}}{dt^2} = \frac{d^2u}{dt^2} \cdot \frac{1}{\sqrt{1 - u^2}} + \frac{u \left( \frac{du}{dt} \right)^2}{(1 - u^2)^{3/2}} \f$
 *
 * - \f$ \text{Dec} = \arctan2(gy, gx) \f$
 * - \f$ \frac{d^2 \text{Dec}}{dt^2} = \frac{(gx \cdot \gamma_y - gy \cdot \gamma_x)(gx^2 + gy^2) - (gx \cdot gpy - gy \cdot gpx)(2gx \cdot gpx + 2gy \cdot gpy)}{(gx^2 + gy^2)^2} \f$
 *
 * @param gx composante x du champ magnétique
 * @param gy composante y du champ magnétique
 * @param gz composante z du champ magnétique
 * @param gpx dérivée de gx
 * @param gpy dérivée de gy
 * @param gpz dérivée de gz
 * @param gamma_x dérivée seconde de gx
 * @param gamma_y dérivée seconde de gy
 * @param gamma_z dérivée seconde de gz
 * @param[out] d2Fdt2 dérivée seconde de F (en nT/s²)
 * @param[out] d2Incdt2 dérivée seconde de l'inclinaison (en degrés/s²)
 * @param[out] d2Decdt2 dérivée seconde de la déclinaison (en degrés/s²)
 */
void computeSecondDerivatives(
    double gx, double gy, double gz,
    double gpx, double gpy, double gpz,
    double gamma_x, double gamma_y, double gamma_z,
    double& d2Incdt2, double& d2Decdt2, double& d2Fdt2
    )
{
    double F = std::sqrt(gx * gx + gy * gy + gz * gz);

    if (F == 0.0) {
        d2Fdt2 = d2Incdt2 = d2Decdt2 = 0.0;
        return;
    }

    // 1. dF/dt
    double dFdt = (gx * gpx + gy * gpy + gz * gpz) / F;

    // 2. d²F/dt²
    double num1 = gpx * gpx + gpy * gpy + gpz * gpz;
    double num2 = gx * gamma_x + gy * gamma_y + gz * gamma_z;
    double dFdt_squared = dFdt * dFdt;
    d2Fdt2 = (num1 + num2) / F - dFdt_squared / F;

    // 3. d²Inc/dt²
    double u = gz / F;
    double du_dt = (gpz * F - gz * dFdt) / (F * F);
    double denomInc = std::sqrt(1 - u * u);
    double d2u_dt2 = (gamma_z * F - 2 * gpz * dFdt - gz * d2Fdt2) / (F * F)
                     + 2 * gz * dFdt * dFdt / (F * F * F);

    if (denomInc == 0.0) {
        d2Incdt2 = 0.0;  // gére cas limite
    } else {
        d2Incdt2 = d2u_dt2 / denomInc + u * du_dt * du_dt / std::pow(1 - u * u, 1.5);
    }

    // 4. d²Dec/dt²
    double N = gx * gpy - gy * gpx;
    double D = gx * gx + gy * gy;
    double dNdt = gx * gamma_y - gy * gamma_x;
    double dDdt = 2 * (gx * gpx + gy * gpy);

    if (D == 0.0) {
        d2Decdt2 = 0.0;
    } else {
        d2Decdt2 = (dNdt * D - N * dDdt) / (D * D);
    }

    // Convertir Inc et Dec en degrés/s²
    d2Incdt2 *= 180.0 * M_1_PI;;
    d2Decdt2 *= 180.0 * M_1_PI;;
}

PosteriorMeanG conversionIDF(const std::vector<double>& vecGx, const std::vector<double>& vecGy, const std::vector<double>& vecGz, const std::vector<double>& vecGxErr, const std::vector<double> &vecGyErr, const std::vector<double> &vecGzErr)
{
    constexpr double deg = 180.0 * M_1_PI;;
    auto n = vecGx.size();
    PosteriorMeanG res;
    res.gx.vecG.resize(n);
    res.gx.vecVarG.resize(n);
    res.gy.vecG.resize(n);
    res.gy.vecVarG.resize(n);
    res.gz.vecG.resize(n);
    res.gz.vecVarG.resize(n);

    for (unsigned long j = 0; j < n ; ++j) {
        const double& Gx = vecGx.at(j);

        const double& Gy = vecGy.at(j);
        const double& Gz = vecGz.at(j);

        //const double F = sqrt(pow(Gx, 2.) + pow(Gy, 2.) + pow(Gz, 2.));
        //const double Inc = asin(Gz / F);
        //const double Dec = atan2(Gy, Gx); // angleD(Gx, Gy);
        double Inc, Dec, F;
        convertToIDF(Gx, Gy, Gz, Inc, Dec, F); // return deg

        // U_cmt_change_repere , ligne 470
        // sauvegarde des erreurs sur chaque paramètre  - on convertit en degrès pour I et D
        // Calcul de la boule d'erreur moyenne par moyenne quadratique ligne 464
       /*   ErrGx:=Tab_parametrique[iJ].ErrGx;
          ErrGy:=Tab_parametrique[iJ].ErrGy;
          ErrGz:=Tab_parametrique[iJ].ErrGz;
          ErrIDF:=sqrt((sqr(ErrGx)+sqr(ErrGy)+sqr(ErrGz))/3);
        */


        const double ErrIDF = sqrt((pow(vecGxErr.at(j), 2.) +pow(vecGyErr.at(j), 2.) +pow(vecGzErr.at(j), 2.))/3.);

        const double ErrI = ErrIDF / F ;
        const double ErrD = ErrIDF / (F * cos(Inc/deg)) ;

       /* long double ErrI = Gz+ErrIDF ; // dans l'espace 3D, l'enveloppe supérieure
        ErrI = abs(asin(ErrIDF/F) - Inc); // pour retrouver la différence

       // long double ErrD = Gz+ErrIDF/F / (F * cos(Inc))
       */
        res.gx.vecG[j] = std::move(Inc);// * deg);
        res.gx.vecVarG[j] = std::move(ErrI* deg);


        res.gy.vecG[j] = std::move(Dec);// * deg);
        res.gy.vecVarG[j] = std::move(ErrD* deg);

        res.gz.vecG[j] = std::move(F);
        res.gz.vecVarG[j] = std::move(ErrIDF);

    }

    auto gmaxF = sqrt(pow(res.gx.mapG.maxY(), 2.) + pow(res.gy.mapG.maxY(), 2.) + pow(res.gz.mapG.maxY(), 2.));
    auto gFMax = asin(res.gz.mapG.maxY() / gmaxF);
    const double gIncMax = asin(res.gz.mapG.maxY() / gmaxF);
    const double gDecMax = atan2(res.gy.mapG.maxY(),res.gx.mapG.maxY());

    auto gminF = sqrt(pow(res.gx.mapG.minY(), 2.) + pow(res.gy.mapG.minY(), 2.) + pow(res.gz.mapG.minY(), 2.));
    auto gFMin = asin(res.gz.mapG.minY() / gminF);
    const double gIncMin = asin(res.gz.mapG.minY() / gminF);
    const double gDecMin = atan2(res.gy.mapG.minY(),res.gx.mapG.minY());


    res.gx.mapG.setRangeY(gIncMin, gIncMax);
    res.gy.mapG.setRangeY(gDecMin, gDecMax);
    res.gz.mapG.setRangeY(gFMin, gFMax);

    return res;
}

void conversionIDF (PosteriorMeanG& G)
{
   constexpr double deg = 180.0 * M_1_PI; ;

   //PosteriorMeanG& res = G;
   auto& vecGx = G.gx.vecG;
   auto& vecGy = G.gy.vecG;
   auto& vecGz = G.gz.vecG;

   auto& vecGxErr = G.gx.vecVarG;
   auto& vecGyErr = G.gy.vecVarG;
   auto& vecGzErr = G.gz.vecVarG;

   const unsigned long n = vecGx.size();
  /* res.gx.vecG.resize(n);
   res.gx.vecVarG.resize(n);
   res.gy.vecG.resize(n);
   res.gy.vecVarG.resize(n);
   res.gz.vecG.resize(n);
   res.gz.vecVarG.resize(n);*/

   for (unsigned long j = 0; j < n ; ++j) {
       const double& Gx = vecGx.at(j);

       const double& Gy = vecGy.at(j);
       const double& Gz = vecGz.at(j);

       const double F = sqrt(pow(Gx, 2.) + pow(Gy, 2.) + pow(Gz, 2.));
       const double Inc = asin(Gz / F);
       const double Dec = atan2(Gy, Gx); // angleD(Gx, Gy);
       // U_cmt_change_repere , ligne 470
       // sauvegarde des erreurs sur chaque paramètre  - on convertit en degrès pour I et D
       // Calcul de la boule d'erreur moyenne par moyenne quadratique ligne 464
      /*   ErrGx:=Tab_parametrique[iJ].ErrGx;
         ErrGy:=Tab_parametrique[iJ].ErrGy;
         ErrGz:=Tab_parametrique[iJ].ErrGz;
         ErrIDF:=sqrt((sqr(ErrGx)+sqr(ErrGy)+sqr(ErrGz))/3);
       */


       const double ErrIDF = sqrt((pow(vecGxErr.at(j), 2.) + pow(vecGyErr.at(j), 2.) + pow(vecGzErr.at(j), 2.))/3.);

       const double ErrI = ErrIDF / F ;
       const double ErrD = ErrIDF / (F * cos(Inc)) ;

      /* long double ErrI = Gz+ErrIDF ; // dans l'espace 3D, l'enveloppe supérieure
       ErrI = abs(asin(ErrIDF/F) - Inc); // pour retrouver la différence

      // long double ErrD = Gz+ErrIDF/F / (F * cos(Inc))
      */
       G.gx.vecG[j] = std::move(Inc * deg);
       G.gx.vecVarG[j] = std::move(ErrI* deg);


       G.gy.vecG[j] = std::move(Dec * deg);
       G.gy.vecVarG[j] = std::move(ErrD* deg);

       G.gz.vecG[j] = std::move(F);
       G.gz.vecVarG[j] = std::move(ErrIDF);

   }

   // Conversion of the map
   // 1 - nouveau extrenum
   const double gzFmax = sqrt(pow(G.gx.mapG.maxY(), 2.) + pow(G.gy.mapG.maxY(), 2.) + pow(G.gz.mapG.maxY(), 2.));
   const double gxIncMax = asin(G.gz.mapG.maxY() / gzFmax);
   const double gyDecMax = atan2(G.gy.mapG.maxY(), G.gx.mapG.maxY());

   const double gzFmin = sqrt(pow(G.gx.mapG.minY(), 2.) + pow(G.gy.mapG.minY(), 2.) + pow(G.gz.mapG.minY(), 2.));
   const double gxIncMin = asin(G.gz.mapG.minY() / gzFmin);
   const double gyDecMin = atan2(G.gy.mapG.minY(), G.gx.mapG.minY());


   G.gx.mapG.setRangeY(gxIncMin * deg, gxIncMax * deg);
   G.gy.mapG.setRangeY(gyDecMin * deg, gyDecMax * deg);
   G.gz.mapG.setRangeY(std::move(gzFmin), std::move(gzFmax));




   // -----------------
/*   G.gx.vecG = std::move(res.gx.vecG);
   G.gy.vecG = std::move(res.gy.vecG);
   G.gz.vecG = std::move(res.gz.vecG);

   G.gx.vecVarG = std::move(res.gx.vecVarG);
   G.gy.vecVarG = std::move(res.gy.vecVarG);
   G.gz.vecVarG = std::move(res.gz.vecVarG);

   G.gx.mapG = std::move(res.gx.mapG);
   G.gy.mapG = std::move(res.gy.mapG);
   G.gz.mapG = std::move(res.gz.mapG);*/
}

/**
 * @brief conversionID identic to conversionIDF
 * @param vecGx
 * @param vecGy
 * @param vecGz
 * @param vecGErr
 * @return
 */
PosteriorMeanG conversionID(const std::vector<double>& vecGx, const std::vector<double>& vecGy, const std::vector<double>& vecGz, const std::vector<double>& vecGxErr, const std::vector<double>& vecGyErr, const std::vector<double>& vecGzErr)
{
   return conversionIDF(vecGx, vecGy, vecGz, vecGxErr, vecGyErr, vecGzErr);
}

void conversionID (PosteriorMeanG& G)
{
    PosteriorMeanG res = conversionID(G.gx.vecG, G.gy.vecG, G.gz.vecG, G.gx.vecVarG, G.gy.vecVarG, G.gz.vecVarG );
    G.gx.vecG = std::move(res.gx.vecG);
    G.gy.vecG = std::move(res.gy.vecG);
    G.gz.vecG = std::move(res.gz.vecG);

    G.gx.vecVarG = std::move(res.gx.vecVarG);
    G.gy.vecVarG = std::move(res.gy.vecVarG);
    G.gz.vecVarG = std::move(res.gz.vecVarG);
}

// Obsolete

QDataStream &operator<<( QDataStream& stream, const MCMCSplineComposante& splineComposante )
{
    stream << (quint32) splineComposante.vecThetaReduced.size();
    for (auto& v : splineComposante.vecThetaReduced)
        stream << (double)v;

    stream << (quint32) splineComposante.vecG.size();
    for (auto& v : splineComposante.vecG)
        stream << (double)v;

    stream << (quint32) splineComposante.vecGamma.size();
    for (auto& v : splineComposante.vecGamma)
        stream << (double)v;

    stream << (quint32) splineComposante.vecVarG.size();
    for (auto& v : splineComposante.vecVarG)
        stream << (double)v;

    return stream;
}

QDataStream &operator>>( QDataStream& stream, MCMCSplineComposante& splineComposante )
{
    quint32 siz;
    double v;
    stream >> siz;

    splineComposante.vecThetaReduced.resize(siz);
    std::generate_n(splineComposante.vecThetaReduced.begin(), siz, [&stream, &v]{stream >> v; return v;});

    stream >> siz;
    splineComposante.vecG.resize(siz);
    std::generate_n(splineComposante.vecG.begin(), siz, [&stream, &v]{stream >> v; return v;});

    stream >> siz;
    splineComposante.vecGamma.resize(siz);
    std::generate_n(splineComposante.vecGamma.begin(), siz, [&stream, &v]{stream >> v; return v;});

    stream >> siz;
    splineComposante.vecVarG.resize(siz);
    std::generate_n(splineComposante.vecVarG.begin(), siz, [&stream, &v]{stream >> v; return v;});

    return stream;
};

QDataStream &operator<<( QDataStream& stream, const MCMCSpline& spline )
{
    stream << spline.splineX;
    stream << spline.splineY;
    stream << spline.splineZ;

    return stream;
}

QDataStream &operator>>( QDataStream& stream, MCMCSpline& spline )
{
    stream >> spline.splineX;
    stream >> spline.splineY;
    stream >> spline.splineZ;

    return stream;
}

QDataStream &operator<<( QDataStream& stream, const PosteriorMeanGComposante& pMGComposante )
{
    stream << (quint32) pMGComposante.vecG.size();
    for (auto& v : pMGComposante.vecG)
        stream << (double) v;

    stream << (quint32) pMGComposante.vecGP.size();
    for (auto& v : pMGComposante.vecGP)
        stream << (double)v;

    stream << (quint32) pMGComposante.vecGS.size();
    for (auto& v : pMGComposante.vecGS)
        stream << (double)v;

    stream << (quint32) pMGComposante.vecVarG.size();
    for (auto& v : pMGComposante.vecVarG)
        stream << (double)v;

    stream << pMGComposante.mapG;

    stream << pMGComposante.mapGP; // since v3.2.7
    return stream;
}

QDataStream &operator>>( QDataStream& stream, PosteriorMeanGComposante& pMGComposante )
{
    quint32 siz;
    double v;

    stream >> siz;
    pMGComposante.vecG.resize(siz);
    std::generate_n(pMGComposante.vecG.begin(), siz, [&stream, &v]{stream >> v; return v;});

    stream >> siz;
    pMGComposante.vecGP.resize(siz);
    std::generate_n(pMGComposante.vecGP.begin(), siz, [&stream, &v]{stream >> v; return v;});

/*    stream >> siz;
    pMGComposante.vecVarGP.resize(siz);
    std::generate_n(pMGComposante.vecVarGP.begin(), siz, [&stream, &v]{stream >> v; return v;});
*/
    stream >> siz;
    pMGComposante.vecGS.resize(siz);
    std::generate_n(pMGComposante.vecGS.begin(), siz, [&stream, &v]{stream >> v; return v;});

    stream >> siz;
    pMGComposante.vecVarG.resize(siz);
    std::generate_n(pMGComposante.vecVarG.begin(), siz, [&stream, &v]{stream >> v; return v;});

    stream >> pMGComposante.mapG;
    if (res_file_version > "3.2.6")
        stream >> pMGComposante.mapGP;
    return stream;
}

QDataStream &operator<<( QDataStream &stream, const PosteriorMeanG& pMeanG )
{
    stream << pMeanG.gx;
    stream << pMeanG.gy;
    stream << pMeanG.gz;

    return stream;
}

QDataStream &operator>>( QDataStream &stream, PosteriorMeanG& pMeanG )
{
    stream >> pMeanG.gx;
    stream >> pMeanG.gy;
    stream >> pMeanG.gz;

    return stream;
}


#pragma mark Calcul Spline on Event


/**
 * @brief MCMCLoopCurve::prepareCalculSpline_WI
 * With W = identity
 * @param vecH
 * @return
 */


SplineMatrices prepare_calcul_spline_WI(const std::vector<t_reduceTime>& vecH)
{
    const SparseMatrixLD R = calculMatR(vecH);
    const SparseMatrixLD Q = calculMatQ(vecH);

    // Calcul de la transposée QT de la matrice Q, de dimension (n-2) x n

    const SparseMatrixLD QT = Q.transpose();//transpose(Q, 3);

    // Calcul de la matrice matQTW_1Q, de dimension (n-2) x (n-2) pour calcul Mat_B
    // matQTW_1Q possèdera 3+3-1=5 bandes
    DiagonalMatrixLD diagWInv;
    diagWInv.setIdentity(vecH.size()+1);


    // Matrix2D matQTW_1Qb = multiMatParMat(matQT, matQ, 3, 3);
    const Matrix2D matQTW_1Q = QT * Q; //multiplyMatrixBanded_Winograd(QT, Q, 1);

    // Calcul de la matrice QTQ, de dimension (n-2) x (n-2) pour calcul Mat_B
    // Mat_QTQ possèdera 3+3-1=5 bandes
    // Matrix2D matQTQb = multiMatParMat(matQT, matQ, 3, 3);
    Matrix2D matQTQ = matQTW_1Q;// multiplyMatrixBanded_Winograd(matQT, matQ, 1); // pHd :: ?? on fait deux fois le même calcul !!

    SplineMatrices matrices;
    matrices.diagWInv = diagWInv;
    matrices.matR = R;
    matrices.matQ = Q;
    //matrices.matQT = QT;
    matrices.matQTW_1Q = matQTW_1Q;
    matrices.matQTQ = matQTQ;

    return matrices;
}


/**
 * @brief MCMCLoopCurve::prepareCalculSpline
 * produit
 * SplineMatrices matrices;
 *  matrices.diagWInv  // utilise mW
 *  matrices.matR   // utilise vecH
 *  matrices.matQ   // utilise vecH
 *  matrices.matQT
 *  matrices.matQTW_1Q  // Seule affectée par changement de VG
 *  matrices.matQTQ
 *
 * @param sortedEvents
 * @return
 */
SplineMatrices prepare_calcul_spline(const std::vector<std::shared_ptr<Event>>& sortedEvents, const std::vector<t_reduceTime>& vecH)
{
    DiagonalMatrixLD W_1 (sortedEvents.size());
    std::transform(sortedEvents.begin(), sortedEvents.end(), W_1.diagonal().begin(), [](std::shared_ptr<Event> ev) {return 1.0L /ev->mW;});

    return prepare_calcul_spline(vecH, W_1);


}

SplineMatrices prepare_calcul_spline(const std::vector<t_reduceTime>& vecH, const DiagonalMatrixLD& W_1)
{
    const SparseMatrixLD R = calculMatR(vecH); // SparseMatrix, mais mémorisé en matrice dense


    const SparseMatrixLD Q = calculMatQ(vecH); // SparseMatrix
    //showMatrix(matQ, "matQ in prepare_calcul_spline");

    // Calcul de la transposée QT de la matrice Q, de dimension (n-2) x n
    const SparseMatrixLD Qt =  Q.transpose();//  transpose(matQ, 3);

    // Calcul de la matrice matQTW_1Q, de dimension (n-2) x (n-2) pour calcul Mat_B
    // matQTW_1Q possèdera 3+3-1=5 bandes
    const SparseMatrixLD QtW_1 = Qt * W_1;// multiMatParDiag(QT, W_1, 3);
    //showMatrix(tmp, "tmp Qt*W");
    //showMatrix(matQT * diagWInv, "direct qt*winv"); //identique

    //Matrix2D matQTW_1Qb = multiMatParMat(tmp, matQ, 3, 3);
    const SparseMatrixLD QtW_1Q = QtW_1 * Q; //multiplyMatrixBanded_Winograd(tmp, Q, 1);
    //showMatrix(matQTW_1Q, "matQTW_1Q=");
    //showMatrix(tmp * matQ, "direct qtw1*q"); // identique

    // Calcul de la matrice QTQ, de dimension (n-2) x (n-2) pour calcul Mat_B
    // Mat_QTQ possèdera 3+3-1=5 bandes
    const SparseMatrixLD QtQ =  Qt * Q; //multiplyMatrixBanded_Winograd(QT, Q, 1);
    //showMatrix(matQTQ, "matQTQ=");
    //showMatrix(matQT * matQ, "direct Qt*Q"); // identique

    SplineMatrices matrices;
    matrices.diagWInv = std::move(W_1);
    matrices.matR = R;
    matrices.matQ = Q;
   // matrices.matQT = Qt;
    matrices.matQTW_1Q = QtW_1Q.toDense(); // Seule affectée par changement de VG
    matrices.matQTQ = QtQ.toDense();

    return matrices;
}

// On utilise la copie au passage du parametre spline_matrices
SplineMatrices update_splineMatrice_with_vecH(SplineMatrices spline_matrices, const std::vector<t_reduceTime>& vecH)
{
    spline_matrices.matR = calculMatR(vecH);
    spline_matrices.matQ = calculMatQ(vecH);

    // Calcul de la transposée QT de la matrice Q, de dimension (n-2) x n
    auto Qt = spline_matrices.matQ.transpose();//transpose(spline_matrices.matQ, 3);

    //MatrixDiag diagWInv (sortedEvents.size());
    //std::transform(sortedEvents.begin(), sortedEvents.end(), diagWInv.begin(), [](std::shared_ptr<Event> ev) {return 1.0 /ev->mW;});

    // Calcul de la matrice matQTW_1Q, de dimension (n-2) x (n-2) pour calcul Mat_B
    // matQTW_1Q possèdera 3+3-1=5 bandes
    const Matrix2D tmp = Qt * spline_matrices.diagWInv ; //multiMatParDiag(spline_matrices.matQT, spline_matrices.diagWInv, 3);

    spline_matrices.matQTW_1Q = tmp * spline_matrices.matQ ; //multiplyMatrixBanded_Winograd(tmp, spline_matrices.matQ, 1); // Seule affectée par changement de VG

    // Calcul de la matrice QTQ, de dimension (n-2) x (n-2) pour calcul Mat_B
    // Mat_QTQ possèdera 3+3-1=5 bandes
    spline_matrices.matQTQ  = Qt * spline_matrices.matQ; //multiplyMatrixBanded_Winograd(spline_matrices.matQT, spline_matrices.matQ, 1);

    return spline_matrices;
}

SplineMatrices update_splineMatrice_with_mW(SplineMatrices spline_matrices, const std::vector<std::shared_ptr<Event> >& sortedEvents)
{

    DiagonalMatrixLD W_1 (sortedEvents.size());
    std::transform(sortedEvents.begin(), sortedEvents.end(), W_1.diagonal().begin(), [](std::shared_ptr<Event> ev) {return 1.0L /ev->mW;});

    spline_matrices.diagWInv = W_1;
    // Calcul de la matrice matQTW_1Q, de dimension (n-2) x (n-2) pour calcul Mat_B
    // matQTW_1Q possèdera 3+3-1=5 bandes
    const Matrix2D tmp = spline_matrices.matQ.transpose() * W_1 ; //multiMatParDiag(spline_matrices.matQT, W_1, 3);

    //Matrix2D matQTW_1Qb = multiMatParMat(tmp, matQ, 3, 3);
    spline_matrices.matQTW_1Q = tmp * spline_matrices.matQ; //multiplyMatrixBanded_Winograd(tmp, spline_matrices.matQ, 1);


    return spline_matrices;
}

SplineMatrices prepareCalculSpline_Sy2(const std::vector<std::shared_ptr<Event>> &sortedEvents, const std::vector<t_reduceTime> &vecH)
{
    const SparseMatrixLD R = calculMatR(vecH);
    const SparseMatrixLD Q = calculMatQ(vecH);

    // Calcul de la transposée QT de la matrice Q, de dimension (n-2) x n
    const auto Qt = Q.transpose();// transpose(Q, 3);

    DiagonalMatrixLD W_1 (sortedEvents.size());
    std::transform(sortedEvents.begin(), sortedEvents.end(), W_1.diagonal().begin(), [](std::shared_ptr<Event> ev) {return ev->mSy * ev->mSy ;});

    // Calcul de la matrice matQTW_1Q, de dimension (n-2) x (n-2) pour calcul Mat_B
    // matQTW_1Q possèdera 3+3-1=5 bandes
    const Matrix2D tmp = Qt * W_1; //multiMatParDiag(QT, W_1, 3);

    //Matrix2D matQTW_1Qb = multiMatParMat(tmp, matQ, 3, 3);
    const Matrix2D QTW_1Q = tmp * Q; //multiplyMatrixBanded_Winograd(tmp, Q, 1);

    // Calcul de la matrice QTQ, de dimension (n-2) x (n-2) pour calcul Mat_B
    // Mat_QTQ possèdera 3+3-1=5 bandes
    //Matrix2D matQTQb = multiMatParMat(matQT, matQ, 3, 3);
    const Matrix2D QTQ = Qt * Q; //multiplyMatrixBanded_Winograd(QT, Q, 1);

    SplineMatrices matrices;
    matrices.diagWInv = std::move(W_1);
    matrices.matR = std::move(R);
    matrices.matQ = std::move(Q);
    //matrices.matQT = std::move(QT);
    matrices.matQTW_1Q = std::move(QTW_1Q); // Seule affectée par changement de VG
    matrices.matQTQ = std::move(QTQ);

    return matrices;
}

SplineResults do_spline(const std::function <t_matrix (std::shared_ptr<Event>)> &fun, const SplineMatrices &matrices, const std::vector<std::shared_ptr<Event>> &events, const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, DiagonalMatrixLD> &decomp_B, const double lambdaSpline)
{
    const size_t n = events.size();
    std::vector<t_matrix> vec_Y (n);
    std::transform(events.begin(), events.end(), vec_Y.begin(), fun);

    return do_spline(vec_Y, matrices, vecH, decomp_B, lambdaSpline);
}


SplineResults doSplineX(const SplineMatrices &matrices, const std::vector<std::shared_ptr<Event> > &events, const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, DiagonalMatrixLD > &decomp_B, const double lambdaSpline)
{
    return do_spline(get_Yx, matrices, events, vecH, decomp_B, lambdaSpline);
}

SplineResults doSplineY(const SplineMatrices &matrices, const std::vector<std::shared_ptr<Event>> &events, const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, DiagonalMatrixLD> &decomp_B, const double lambdaSpline)
{
    return do_spline(get_Yy, matrices, events, vecH, decomp_B, lambdaSpline);
}

SplineResults doSplineZ(const SplineMatrices &matrices, const std::vector<std::shared_ptr<Event> > &events, const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, DiagonalMatrixLD > &decomp_B, const double lambdaSpline)
{
    return do_spline(get_Yz, matrices, events, vecH, decomp_B, lambdaSpline);
}

/**
 Cette procedure calcule la matrice inverse de B:
 B = R + lambda * Qt * W-1 * Q
 puis calcule la matrice d'influence A(lambda), SEULEMENT les termes diagonaux
 Bande : nombre de diagonales non nulles
 used only with MCMCLoopCurve::calcul_spline_variance()
*/

// plus lent à cause de la fonction matQ.coeff(0, 1)
DiagonalMatrixLD diagonal_influence_matrix_old(const SplineMatrices& matrices, const int nbBandes, const std::pair<Matrix2D, DiagonalMatrixLD> &decomp, const double lambda)
{

    const size_t n = matrices.diagWInv.rows();

    const Matrix2D &matB_1 = inverseMatSym_origin(decomp, nbBandes + 4, 1);

    DiagonalMatrixLD matQB_1QT (n);

    /*Calcul des termes diagonaux de Q (matB_1) Qt, de i=1 à nb_noeuds
             ??? vérifier le contrôle des indices i+1: débordement de matrices */


    const auto& matQ = matrices.matQ; //std::begin(matrices.matQ[0]); // matQ est une matrice encadrée de 0

    t_matrix term = std::pow(matrices.matQ.coeff(0, 0), 2.0L) * matB_1(0, 0);
    term += std::pow(matQ.coeff(0, 1), 2.0L) * matB_1(1, 1);
    term += 2.0L * matQ.coeff(0, 0) * matQ.coeff(0, 1) * matB_1(0, 1);


    matQB_1QT.diagonal()[0] = term;

    for (size_t i = 1; i < n-1; ++i) {

        t_matrix term_i = std::pow(matQ.coeff(i, i-1), 2.0L) * matB_1(i-1, i-1);
        term_i += std::pow(matQ.coeff(i, i), 2.0L) * matB_1(i, i);
        term_i += std::pow(matQ.coeff(i, i+1), 2.0L) * matB_1(i+1, i+1);

        term_i += 2.0L * matQ.coeff(i, i-1) * matQ.coeff(i, i) * matB_1(i-1, i);
        term_i += 2.0L * matQ.coeff(i, i-1) * matQ.coeff(i, i+1) * matB_1(i-1, i+1);
        term_i += 2.0L * matQ.coeff(i, i) * matQ.coeff(i, i+1) * matB_1(i, i+1);


        matQB_1QT.diagonal()[i] = term_i;
    }


    term = std::pow(matQ.coeff(n-1, n-2), 2.0L) * matB_1(n-2, n-2);
    term += std::pow(matQ.coeff(n-1, n-1), 2.0L) * matB_1(n-1, n-1);
    term += 2.0L * matQ.coeff(n-1, n-2) * matQ.coeff(n-1, n-1) * matB_1(n-2, n-1);

    matQB_1QT.diagonal()[n-1] = term;

    DiagonalMatrixLD matA (n);

    long double lambda_L = static_cast<long double>(lambda);
    for (size_t i = 0; i < n; ++i) {
        long double mat_a = 1.0L - lambda_L * static_cast<long double>(matrices.diagWInv.diagonal()[i]) * matQB_1QT.diagonal()[i];
        matA.diagonal()[i] = std::clamp(mat_a, 0.0L, 1.0L);
    }

    return matA;
}

DiagonalMatrixLD diagonal_influence_matrix(const SplineMatrices& matrices,
                                           const int nbBandes,
                                           const std::pair<Matrix2D, DiagonalMatrixLD> &decomp,
                                           const double lambda)
{
    const Index n = matrices.diagWInv.rows();

    const Matrix2D &matB_1 = inverseMatSym_origin(decomp, nbBandes + 4, 1);

    DiagonalMatrixLD matQB_1QT(n);

    // --- Extraire les trois diagonales de matQ (tridiagonale) dans des vecteurs contigus ---
    std::vector<t_matrix> q_diag(n, 0.0L);
    std::vector<t_matrix> q_sup(n > 1 ? n-1 : 0, 0.0L); // (i,i+1)
    std::vector<t_matrix> q_sub(n > 1 ? n-1 : 0, 0.0L); // (i,i-1)

    // Parcours efficace de la sparse matrix pour remplir diag/sup/sub
    for (Index k = 0; k < matrices.matQ.outerSize(); ++k) {
        for (typename SparseMatrix<t_matrix>::InnerIterator it(matrices.matQ, k); it; ++it) {
            const Index i = it.row();
            const Index j = it.col();
            const t_matrix v = it.value();
            if (i == j) {
                q_diag[i] = v;

            } else if (j == i+1) {
                q_sup[i] = v;      // element (i,i+1) stored at index i

            } else if (j+1 == i) {
                q_sub[j] = v;      // element (i,i-1) => sub at index i-1 stored at j
            }
            // On suppose tridiagonale : autres éléments ignorés
        }
    }

    // --- Extraire diagonales / bandes nécessaires de matB_1 (dense) ---
    std::vector<t_matrix> B_diag(n, 0.0L);
    std::vector<t_matrix> B_sup(n > 1 ? n-1 : 0, 0.0L);    // B(i,i+1)
    std::vector<t_matrix> B_i1i1(n > 1 ? n-1 : 0, 0.0L);   // B(i-1,i-1) stocké à i-1 (utilisé pour lecture claire)
    std::vector<t_matrix> B_i2(n > 2 ? n-2 : 0, 0.0L);     // B(i-1,i+1) stored at i-1 when needed

    for (Index i = 0; i < n; ++i) {
        B_diag[i] = matB_1(i, i);
        if (i + 1 < n) {
            B_sup[i] = matB_1(i, i+1);
            B_i1i1[i] = matB_1(i, i); // same as B_diag but keep for clarity
        }
        if (i + 2 < n) {
            // B(i, i+2) corresponds to matB_1(i, i+2) used when accessing B(i-1,i+1)
            B_i2[i] = matB_1(i, i+2);
        }
    }
    // Note: For B(i-1,i+1) we'll read B_i2[i-1] (i>=1 && i+1 < n => i-1 <= n-3 => valid)

    // --- Calcul du diag matQB_1QT sans appels répétés à coeff() ---
    // premier élément i = 0
    if (n > 0) {
        t_matrix term0 = q_diag[0]*q_diag[0]*B_diag[0];
        if (n > 1) {
            term0 += q_sup[0]*q_sup[0]*B_diag[1];
            term0 += 2.0L * q_diag[0] * q_sup[0] * B_sup[0];
        }
        matQB_1QT.diagonal()[0] = term0; // utilisation Eigen API pour écrire le diag
    }

    // éléments intermédiaires 1 .. n-2
    for (Index i = 1; i + 1 < n; ++i) {
        // indices auxiliaires
        const Index im1 = i - 1;
        const Index ip1 = i + 1;

        t_matrix a = q_sub[im1] * q_sub[im1] * B_diag[im1];   // q(i,i-1)^2 * B(i-1,i-1)
        a += q_diag[i]*q_diag[i] * B_diag[i];               // q(i,i)^2 * B(i,i)
        a += q_sup[i]*q_sup[i] * B_diag[ip1];               // q(i,i+1)^2 * B(i+1,i+1)

        a += 2.0L * q_sub[im1] * q_diag[i] * B_sup[im1];    // 2*q(i,i-1)*q(i,i)*B(i-1,i)
        // B(i-1,i+1) correspond à matB_1(i-1, i+1) -> stocké en B_i2[i-1]
        if (im1 < static_cast<Index>( B_i2.size())) {
            a += 2.0L * q_sub[im1] * q_sup[i] * B_i2[im1];
        }
        a += 2.0L * q_diag[i] * q_sup[i] * B_sup[i];        // 2*q(i,i)*q(i,i+1)*B(i,i+1)

        matQB_1QT.diagonal()[i] = a;
    }

    // dernier élément i = n-1
    if (n > 1) {
        size_t i = n - 1;
        t_matrix termN = q_sub[i-1]*q_sub[i-1]*B_diag[i-1];
        termN += q_diag[i]*q_diag[i]*B_diag[i];
        termN += 2.0L * q_sub[i-1] * q_diag[i] * B_sup[i-1];
        matQB_1QT.diagonal()[i] = termN;
    }

    // --- Calcul matA diagonal avec clamp ---
    DiagonalMatrixLD matA(n);
    const t_matrix lambdaL = static_cast<t_matrix>(lambda);

    for (Index i = 0; i < n; ++i) {
        const t_matrix winv = matrices.diagWInv.diagonal()[i];
        const t_matrix qb = matQB_1QT.diagonal()[i];
        t_matrix mat_a = 1.0L - lambdaL * winv * qb;

        if (mat_a < (t_matrix)0) {
            qDebug() << "[CurveUtilities] diagonal_influence_matrix : Oups mat_a=" << static_cast<double>(mat_a) << "< 0 change to 0" << "n=" << n;

            mat_a = 0.0L;
        }
        else if (mat_a > 1.0) {
            qDebug() << "[CurveUtilities] diagonal_influence_matrix : Oups mat_a="<< static_cast<double>(mat_a) << "> 1 change to 1" << "n=" << n;

            mat_a = 1.0L;
        }

        matA.diagonal()[i] = mat_a;
    }

    return matA;
}

std::vector<double> calcul_spline_variance(const SplineMatrices& matrices, const std::vector<std::shared_ptr<Event>> &events, const std::pair<Matrix2D, DiagonalMatrixLD> &decomp, const double lambdaSpline)
{
    unsigned int n = (unsigned int)events.size();
    DiagonalMatrixLD matA = diagonal_influence_matrix(matrices, 1, decomp, lambdaSpline);
    std::vector<double> varG;

    for (unsigned int i = 0; i < n; ++i) {
#ifdef DEBUG
        const double& aii = matA.diagonal()[i];
        // si Aii négatif ou nul, cela veut dire que la variance sur le point est anormalement trop grande,
        // d'où une imprécision dans les calculs de Mat_B (Cf. calcul spline) et de mat_A
        if (aii < 0. || events[i]->mW <0) {
            qDebug()<<"[MCMCLoopCurve] calcul_spline_variance() : Oups aii="<< aii <<"<= 0 change to 0" << "mW="<<events[i]->mW;
            varG.push_back(0.);

        } else {
            varG.push_back(matA.diagonal()[i]  / events[i]->mW);
        }
#else
        varG.push_back(matA.diagonal()[i]  / events[i]->mW);
#endif

    }

    return varG;
}


/**
 * @brief MCMCLoopCurve::currentSpline
 * @param events  update Gx , Gy and Gz of Event
 * @param doSortAndSpreadTheta
 * @param vecH
 * @param matrices
 * @return
 */
MCMCSpline currentSpline (std::vector<std::shared_ptr<Event> > &events, const std::vector<t_reduceTime> &vecH, const SplineMatrices &matrices, const double lambda, bool doY, bool doZ)
{

    const auto vec_theta_red = get_vector<t_reduceTime>(get_ThetaReduced, events);

    // doSpline utilise les Y des events
    // => On le calcule ici pour la première composante (x)

    SparseMatrixLD B; // B de bande 5; matrices.matR de bande 3; matrices.matQTW_1Q de bande 5

    if (lambda != 0) {
        //const Matrix2D tmp = multiConstParMat(matrices.matQTW_1Q, lambda, 5);
        B = matrices.matR + lambda * matrices.matQTW_1Q; // addMatEtMat(matrices.matR, tmp, 5);

    } else {
        B = matrices.matR;
    }

    // test

    // test
   /* Matrix2D D00 (4,4);
    D00 << 0.1, 0.0, 0.0, 0.0,
        0.0, 0.09, 0.0, 0.0,
        0.0, 0.0, 0.0933, 0.0,
        0.0, 0.0, 0.0,    0.15;
    Matrix2D L00 (4, 4);
    L00 << 1.0, 0.0, 0.0, 0.0,
        0.036, 1.0, 0.0, 0.0,
        7.57576e-09, 0.0093, 1.0, 0.0,
        7.0, 7.57576e-09,    0.036, 1.0;

    Matrix2D test = L00 * D00 * L00.transpose();


    Matrix2D A00 = seedMatrix(matB, 1);//L00 * D00 * L00.transpose();

*/


    // Decomposition_Cholesky de matB en matL et matD
    // Si alpha global: calcul de B = R + lambda * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
    const std::pair<Matrix2D, DiagonalMatrixLD> decomp = decompositionCholesky(B.toDense(), 5, 1);


    // le calcul de l'erreur est influencé par VG qui induit 1/mW, utilisé pour fabriquer matrices->DiagWinv et calculer matrices->matQTW_1Q
    // Tout le calcul précédent ne change pas

    const SplineResults sx = doSplineX(matrices, events, vecH, decomp, lambda); // Voir si matB est utile ???
    // Les erreurs sont égales sur les trois composantes X, Y, Z splineY.vecErrG = splineX.vecErrG =

     const std::vector<double> &vec_varG = calcul_spline_variance(matrices, events, decomp, lambda);
   // showVector(vecVarG, "vecVarG");

    // -- autre méthode pour varG, sans passer par l'inversion de B, en utilisant la résolution de l'équation

/*
    DiagonalMatrixLD W_1 (events.size()) ; // correspond à 1.0/mW
    std::transform(events.begin(), events.end(), W_1.diagonal().begin(), [](std::shared_ptr<Event> ev){return 1.0/ ev->mW;});// {return (ev->mSy*ev->mSy + ev->mVg.mX;});

    auto Z = matrices.matQ.transpose() * W_1; // ici

    // autre solution avec solver LLT plus robuste que LDLT
    SparseMatrixLD result(B.rows(), B.cols());
    {
    //Eigen::SimplicialLLT<SparseMatrixLD> solver; // si tu es sûr SPD
    Eigen::SparseLU<SparseMatrixLD> solver;      // fonctionne même si pas SPD

    constexpr int shift_ = 1;
    const auto n_center = B.rows() - 2 * shift_;
    SparseMatrixLD B_center = B.block(shift_, shift_, n_center, n_center);

    solver.compute(B_center); // effectue la factorisation solver
#ifdef DEBUG
    if (solver.info() != Eigen::Success) {
        throw std::runtime_error("[currentSpline] LU factorization failed");
    }

#endif
    SparseMatrixLD Z_relevant = Z.block(shift_, 0, n_center, Z.cols());

    Matrix2D Z_dense = Matrix2D(Z_relevant);
    Matrix2D solutions = solver.solve(Z_dense);

    //showMatrix(solutions, "solutions");
    // Remise en forme avec padding

    std::vector<Eigen::Triplet<t_matrix>> triplets;

    for (int i = 0; i < solutions.rows(); ++i) {
        for (int j = 0; j < solutions.cols(); ++j) {
            const t_matrix value = static_cast<t_matrix>(solutions(i, j));
            triplets.emplace_back(i + shift_, j, value);
        }
    }

    result.setFromTriplets(triplets.begin(), triplets.end());
    result.makeCompressed();
    }
    Matrix2D varG = W_1.toDenseMatrix() - lambda * W_1 * matrices.matQ * result;
    // transtypage
    DiagonalMatrixLD varG_diag = varG.diagonal().asDiagonal();
    std::vector<double> vec_varG(varG_diag.diagonal().data(), varG_diag.diagonal().data() + varG_diag.diagonal().rows());

#ifdef DEBUG
    if (std::any_of(vec_varG.begin(), vec_varG.end(),
                    [](double v){ return v <= 0; })) {
        qDebug() << "[CurveUtilities::currentSpline] Houps! varG < 0 ";
        showVector(vec_varG, "vec_varG2");
    }

#endif
*/
    // ---
/* utilisation d'un solveur LDLT pas assez robust
    SparseQuadraticFormSolver solver(1); // shift=1 notre padding
    solver.factorize(B0); // Factorisation une seule fois, crée le solver ldlt

    Matrix2D B_1QtW_1 = solver.compute_Rinv_QT(Z);

    Matrix2D varG = W_1.toDenseMatrix() - lambda * W_1 * matrices.matQ * B_1QtW_1;
    // transtypage
    DiagonalMatrixLD varG_diag = varG.diagonal().asDiagonal();
    std::vector<double> vec_varG(varG_diag.diagonal().data(), varG_diag.diagonal().data() + varG_diag.diagonal().rows());
    showVector(vec_varG, "vec_varG");
*/
    // --------------------------------------------------------------
    //  Calcul de la spline g, g" pour chaque composante x y z + stockage
    // --------------------------------------------------------------

    MCMCSplineComposante splineX;
    splineX.vecThetaReduced = vec_theta_red;
    splineX.vecG = std::move(sx.vecG);
    splineX.vecGamma = std::move(sx.vecGamma);

    splineX.vecVarG = vec_varG ; //vecVarG;

    for (size_t i = 0; i < events.size(); i++) {
        events[i]->mGx = splineX.vecG[i];
    }

    MCMCSpline spline;
    spline.splineX = std::move(splineX);


    if ( doY) {

        // doSpline utilise les Y des events
        // => On le calcule ici pour la seconde composante (y)

        const SplineResults sy = doSplineY(matrices, events, vecH, decomp, lambda); //matL et matB ne sont pas changés

        MCMCSplineComposante splineY;

        splineY.vecG = std::move(sy.vecG);
        splineY.vecGamma = std::move(sy.vecGamma);
        for (size_t i = 0; i < events.size(); i++) {
            events[i]->mGy = splineY.vecG[i];
        }
        splineY.vecThetaReduced = vec_theta_red;
        splineY.vecVarG = vec_varG; //vecVarG;  // Les erreurs sont égales sur les trois composantes X, Y, Z splineY.vecErrG = splineX.vecErrG =

        spline.splineY = std::move(splineY);
    }

    if ( doZ) {
        // dans le future, ne sera pas utile pour le mode sphérique
        // doSpline utilise les Z des events
        // => On le calcule ici pour la troisième composante (z)

        const SplineResults sz = doSplineZ(matrices, events, vecH, decomp, lambda);

        MCMCSplineComposante splineZ;

        splineZ.vecG = std::move(sz.vecG);
        splineZ.vecGamma = std::move(sz.vecGamma);
        for (size_t i = 0; i < events.size(); i++) {
            events[i]->mGz = splineZ.vecG[i];
        }
        splineZ.vecThetaReduced= vec_theta_red;
        splineZ.vecVarG = vec_varG; //vecVarG;

        spline.splineZ = std::move(splineZ);
    }

    return spline;
}

/**
 * @brief MCMCLoopCurve::currentSpline_WI. Lambda = 0
 * @param events must be ordered and spred
 * @return
 */
MCMCSpline currentSpline_WI (std::vector<std::shared_ptr<Event>> &events, bool doY, bool doZ, bool use_error)
{
    //Q_ASSERT_X(mModel->mLambdaSpline.mX!=0, "[MCMCLoopCurve::ln_h_YWI_3_update]", "lambdaSpline=0");

    const std::vector<t_reduceTime> &vecH = calculVecH(events);
    const SplineMatrices &spline_matrices = prepare_calcul_spline_WI(vecH);


    const auto &vec_theta_red = get_vector<t_reduceTime>(get_ThetaReduced, events);

    // doSpline utilise les Y des events
    // => On le calcule ici pour la première composante (x)

    const Matrix2D &matB  = spline_matrices.matR;

    // Decomposition_Cholesky de matB en matL et matD
    // Si alpha global: calcul de Mat_B = R + alpha * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
    const std::pair<Matrix2D, DiagonalMatrixLD> &decomp = decompositionCholesky(matB, 5, 1);


    // le calcul de l'erreur est influencé par VG qui induit 1/mW, utilisé pour fabriquer matrices->DiagWinv et calculer matrices->matQTW_1Q
    // Tout le calcul précédent ne change pas

    std::vector<double> vecVarG;
    //if (mCurveSettings.mUseErrMesure)
    if (use_error)
        vecVarG = calcul_spline_variance(spline_matrices, events, decomp, 0.); // Les erreurs sont égales sur les trois composantes X, Y, Z splineY.vecErrG = splineX.vecErrG =
    else
        vecVarG = std::vector(events.size(), 0.);
    // --------------------------------------------------------------
    //  Calcul de la spline g, g" pour chaque composante x y z + stockage
    // --------------------------------------------------------------

    const SplineResults &sx = doSplineX(spline_matrices, events, vecH, decomp, 0.);

    MCMCSplineComposante splineX;
    splineX.vecThetaReduced = vec_theta_red;
    splineX.vecG = std::move(sx.vecG);
    splineX.vecGamma = std::move(sx.vecGamma);

    splineX.vecVarG = vecVarG;

    MCMCSpline spline;
    spline.splineX = std::move(splineX);


    if ( doY) {

        const SplineResults &sy = doSplineY(spline_matrices, events, vecH, decomp, 0.); //matL et matB ne sont pas changés

        MCMCSplineComposante splineY;

        splineY.vecG = std::move(sy.vecG);
        splineY.vecGamma = std::move(sy.vecGamma);

        splineY.vecThetaReduced = vec_theta_red;
        splineY.vecVarG = vecVarG;  // Les erreurs sont égales sur les trois composantes X, Y, Z splineY.vecErrG = splineX.vecErrG =

        spline.splineY = std::move(splineY);
    }

    if ( doZ) {
        // dans le future, ne sera pas utile pour le mode sphérique
        // doSpline utilise les Z des events
        // => On le calcule ici pour la troisième composante (z)

        const SplineResults &sz = doSplineZ(spline_matrices, events, vecH, decomp, 0.);

        MCMCSplineComposante splineZ;

        splineZ.vecG = std::move(sz.vecG);
        splineZ.vecGamma = std::move(sz.vecGamma);

        splineZ.vecThetaReduced = vec_theta_red;
        splineZ.vecVarG = vecVarG;

        spline.splineZ = std::move(splineZ);
    }

    return spline;
}


double valeurG(const double t, const MCMCSplineComposante& spline, unsigned long &i0, Model &model)
{
    const auto n = spline.vecThetaReduced.size();
    const auto tReduce = model.reduceTime(t);
    const auto t1 = spline.vecThetaReduced.at(0);
    const auto tn = spline.vecThetaReduced.at(n-1);
    double g = 0;

    if (tReduce < t1) {
        const auto t2 = spline.vecThetaReduced.at(1);
        double gp1 = (spline.vecG.at(1) - spline.vecG.at(0)) / (t2 - t1);
        gp1 -= (t2 - t1) * spline.vecGamma.at(1) / 6.;
        return (double) (spline.vecG.at(0) - (t1 - tReduce) * gp1);

    } else if (tReduce >= tn) {
        const auto tn1 = spline.vecThetaReduced.at(n-2);
        auto gpn = (spline.vecG.at(n-1) - spline.vecG.at(n-2)) / (tn - tn1);
        gpn += (tn - tn1) * spline.vecGamma.at(n-2) / 6.;
        return (double) (spline.vecG.at(n-1) + (tReduce - tn) * gpn);

    } else {
        for (; i0 < n-1; ++i0) {
            const auto ti1 = spline.vecThetaReduced.at(i0);
            const auto ti2 = spline.vecThetaReduced.at(i0+1);
            if ((tReduce >= ti1) && (tReduce < ti2)) {
                const auto h = ti2 - ti1;
                const double gi1 = spline.vecG.at(i0);
                const double gi2 = spline.vecG.at(i0+1);

                // Linear part :
                g = gi1 + (gi2-gi1)*(tReduce-ti1)/h;

                // Smoothing part :
                const auto gamma1 = spline.vecGamma.at(i0);
                const auto gamma2 = spline.vecGamma.at(i0+1);
                const auto p = (1./6.) * ((tReduce-ti1) * (ti2-tReduce)) * ((1.+(tReduce-ti1)/h) * gamma2 + (1.+(ti2-tReduce)/h) * gamma1);
                g -= p;

                break;
            }
        }
    }

    return g;
}

// ------------------------------------------------------------------
//  Valeur de Err_G en t à partir de Vec_Err_G
//  par interpolation linéaire des erreurs entre les noeuds
// ------------------------------------------------------------------
//obsolete
double valeurErrG(const double t, const MCMCSplineComposante& spline, unsigned& i0, Model &model)
{
    const unsigned n = (unsigned) spline.vecThetaReduced.size();
    const double t_red = model.reduceTime(t);

    const double t1 = spline.vecThetaReduced[0];
    const double tn = spline.vecThetaReduced[n-1];
    double errG = 0;

    if (t_red < t1) {
        errG = sqrt(spline.vecVarG.at(0));

    } else if (t_red >= tn) {
        errG = sqrt(spline.vecVarG.at(n-1));

    } else {
        for (; i0 <n-1; ++i0) {
            const auto ti1 = spline.vecThetaReduced[i0];
            const auto ti2 = spline.vecThetaReduced[i0+1];
            if ((t_red >= ti1) && (t < ti2)) {

                const double err1 = sqrt(spline.vecVarG[i0]);
                const double err2 = sqrt(spline.vecVarG[i0+1]);
                errG = err1 + ((t_red-ti1) / (ti2-ti1)) * (err2 - err1);
                break;
            }
        }
    }

    return errG;
}

// dans RenCurve U-CMT-Routine_Spline Valeur_Gp // useless
double valeurGPrime(const double t, const MCMCSplineComposante& spline, unsigned& i0, Model &model)
{
    const unsigned n = (unsigned)spline.vecThetaReduced.size();
    const double tReduce =  model.reduceTime(t);
    const double t1 = spline.vecThetaReduced.at(0);
    const double tn = spline.vecThetaReduced.at(n-1);
    double gPrime = 0.;

    // la dérivée première est toujours constante en dehors de l'intervalle [t1,tn]
    if (tReduce < t1) {
        const double t2 = spline.vecThetaReduced.at(1);
        gPrime = (spline.vecG.at(1) - spline.vecG.at(0)) / (t2 - t1);
        gPrime -= (t2 - t1) * spline.vecGamma.at(1) / 6.;


    } else if (tReduce >= tn) {
        const double tin_1 = spline.vecThetaReduced.at(n-2);
        gPrime = (spline.vecG.at(n-1) - spline.vecG.at(n-2)) / (tn - tin_1);
        gPrime += (tn - tin_1) * spline.vecGamma.at(n-2) / 6.;


    } else {
        for ( ;i0< n-1; ++i0) {
            const auto ti1 = spline.vecThetaReduced.at(i0);
            const auto ti2 = spline.vecThetaReduced.at(i0+1);
            if ((tReduce >= ti1) && (tReduce < ti2)) {
                const auto h = ti2 - ti1;
                const double gi1 = spline.vecG.at(i0);
                const double gi2 = spline.vecG.at(i0+1);
                const double gamma1 = spline.vecGamma.at(i0);
                const double gamma2 = spline.vecGamma.at(i0+1);

                gPrime = ((gi2-gi1)/h) - (1./6.) * (tReduce-ti1) * (ti2-tReduce) * ((gamma2-gamma1)/h);
                gPrime += (1./6.) * ((tReduce-ti1) - (ti2-tReduce)) * ( (1.+(tReduce-ti1)/h) * gamma2 + (1+(ti2-tReduce)/h) * gamma1 );

                break;
            }
        }

    }

    return gPrime;
}

// useless
double valeurGSeconde(const double t, const MCMCSplineComposante& spline, Model& model)
{
    const size_t n = spline.vecThetaReduced.size();
    const auto tReduce = model.reduceTime(t);
    // The second derivative is always zero outside the interval [t1,tn].
    double gSeconde = 0.;

    for (size_t i = 0; i < n-1; ++i) {
        const t_reduceTime ti1 = spline.vecThetaReduced.at(i);
        const t_reduceTime ti2 = spline.vecThetaReduced.at(i+1);
        if ((tReduce >= ti1) && (tReduce < ti2)) {
            const t_reduceTime h = ti2 - ti1;
            const double gamma1 = spline.vecGamma.at(i);
            const double gamma2 = spline.vecGamma.at(i+1);
            gSeconde = ((tReduce-ti1) * gamma2 + (ti2-tReduce) * gamma1) / h;
            break;
        }
    }

    return gSeconde;
}

/**
 * @brief Calcule la valeur de la spline (G), sa variance (varG), sa dérivée première (GP) et seconde (GS), à un instant t donné.
 *
 * @param t Le temps réel.
 * @param spline La spline MCMC contenant les points et dérivées.
 * @param[out] G Valeur de la spline.
 * @param[out] varG Variance associée à G.
 * @param[out] GP Dérivée première.
 * @param[out] GS Dérivée seconde.
 * @param[in,out] i0 Index initial estimé pour l’intervalle du temps réduit (optimisé si appel successif).
 * @param tmin Temps réel minimum de la spline.
 * @param tmax Temps réel maximum de la spline.
 */
void valeurs_G_VarG_GP_GS(const double t, const MCMCSplineComposante &spline, double& G, double& varG, double& GP, double& GS, unsigned& i0, double tmin, double tmax)
{
    constexpr double one_sixth = 1.0 / 6.0;

    const size_t n = spline.vecThetaReduced.size();
    const double invDuration = 1.0 / (tmax - tmin);
    const t_reduceTime tReduce =  (t - tmin) * invDuration;
    const t_reduceTime t1 = spline.vecThetaReduced.front();
    const t_reduceTime tn = spline.vecThetaReduced.back();

    // The first derivative is always constant outside the interval [t1,tn].
    if (tReduce <= t1) {
        const t_reduceTime t2 = spline.vecThetaReduced[1];
        const double h = t2 - t1;

        // ValeurGPrime
        GP = (spline.vecG[1] - spline.vecG[0]) / h - h * spline.vecGamma[1] * one_sixth;

        // ValeurG
        G = spline.vecG[0] - (t1 - tReduce) * GP;

        // valeurErrG
        varG = spline.vecVarG[0];

        // valeurGSeconde
        GS = 0.0;

    } else if (tReduce >= tn) {

        const t_reduceTime tn1 = spline.vecThetaReduced[n-2];
        const double h = tn - tn1;

        GP = (spline.vecG[n-1] - spline.vecG[n-2]) / h + h * spline.vecGamma[n-2] * one_sixth;

        G = spline.vecG[n-1] + (tReduce - tn) * GP;
        varG = spline.vecVarG[n-1];
        GS = 0.0;


    } else {

        for (; i0 < n-1; ++i0) {
            const t_reduceTime ti1 = spline.vecThetaReduced[i0];
            const t_reduceTime ti2 = spline.vecThetaReduced[i0 + 1];


            if ((tReduce >= ti1) && (tReduce < ti2)) {
                const double h = ti2 - ti1;
                const double u = tReduce - ti1;
                const double v = ti2 - tReduce;

                const double gi1 = spline.vecG[i0];
                const double gi2 = spline.vecG[i0 + 1];
                const double gamma1 = spline.vecGamma[i0];
                const double gamma2 = spline.vecGamma[i0 + 1];


                // Spline value
                G = ( u * gi2 + v * gi1 ) / h;
                G -= one_sixth * (u * v) * ((1.0 + u / h) * gamma2 + (1.0 + v / h) * gamma1);

                // Interpolated variance
                const double err1 = sqrt(spline.vecVarG[i0]);
                const double err2 = sqrt(spline.vecVarG[i0 + 1]);
                const double alpha = u / h;

                varG = err1 + alpha * (err2 - err1);
                varG *= varG;

                // First derivative
                GP = ((gi2 - gi1) / h) - one_sixth * u * v * (gamma2 - gamma1) / h;
                GP += one_sixth * (u - v) * ( (1.0 + u / h) * gamma2 + (1.0 + v / h) * gamma1);

                // Second derivative
                GS = (u * gamma2 + v * gamma1) / h;


                break;
            }
        }

    }

    // Rescale derivatives for original time domain
    GP *= invDuration;
    GS *= invDuration * invDuration;


}
/**
 * @brief Calcule la valeur de la spline (G), sa dérivée première (GP) et seconde (GS), à un instant t donné.
 *
 * @param t Le temps réel.
 * @param spline La spline MCMC contenant les points et dérivées.
 * @param[out] G Valeur de la spline.
 * @param[out] GP Dérivée première.
 * @param[out] GS Dérivée seconde.
 * @param[in,out] i0 Index initial estimé pour l’intervalle du temps réduit (optimisé si appel successif).
 * @param tmin Temps réel minimum de la spline.
 * @param tmax Temps réel maximum de la spline.
 */
void valeurs_G_GP_GS(const double t, const MCMCSplineComposante &spline, double& G, double& GP, double& GS, unsigned& i0, double tmin, double tmax)
{
    constexpr double one_sixth = 1.0 / 6.0;

    const size_t n = spline.vecThetaReduced.size();
    const double invDuration = 1.0 / (tmax - tmin);
    const t_reduceTime tReduce =  (t - tmin) * invDuration;
    const t_reduceTime t1 = spline.vecThetaReduced.front();
    const t_reduceTime tn = spline.vecThetaReduced.back();

    // The first derivative is always constant outside the interval [t1,tn].
    if (tReduce <= t1) {
        const t_reduceTime t2 = spline.vecThetaReduced[1];
        const double h = t2 - t1;

        // ValeurGPrime
        GP = (spline.vecG[1] - spline.vecG[0]) / h - h * spline.vecGamma[1] * one_sixth;

        // ValeurG
        G = spline.vecG[0] - (t1 - tReduce) * GP;

        // valeurGSeconde
        GS = 0.0;

    } else if (tReduce >= tn) {

        const t_reduceTime tn1 = spline.vecThetaReduced[n-2];
        const double h = tn - tn1;

        GP = (spline.vecG[n-1] - spline.vecG[n-2]) / h + h * spline.vecGamma[n-2] * one_sixth;

        G = spline.vecG[n-1] + (tReduce - tn) * GP;

        GS = 0.0;


    } else {

        for (; i0 < n-1; ++i0) {
            const t_reduceTime ti1 = spline.vecThetaReduced[i0];
            const t_reduceTime ti2 = spline.vecThetaReduced[i0 + 1];


            if ((tReduce >= ti1) && (tReduce < ti2)) {
                const double h = ti2 - ti1;
                const double u = tReduce - ti1;
                const double v = ti2 - tReduce;

                const double gi1 = spline.vecG[i0];
                const double gi2 = spline.vecG[i0 + 1];
                const double gamma1 = spline.vecGamma[i0];
                const double gamma2 = spline.vecGamma[i0 + 1];


                // Spline value
                G = ( u * gi2 + v * gi1 ) / h;
                G -= one_sixth * (u * v) * ((1.0 + u / h) * gamma2 + (1.0 + v / h) * gamma1);


                // First derivative
                GP = ((gi2 - gi1) / h) - one_sixth * u * v * (gamma2 - gamma1) / h;
                GP += one_sixth * (u - v) * ( (1.0 + u / h) * gamma2 + (1.0 + v / h) * gamma1);

                // Second derivative
                GS = (u * gamma2 + v * gamma1) / h;

                break;
            }
        }

    }

    // Rescale derivatives for original time domain
    GP *= invDuration;
    GS *= invDuration * invDuration;


}


#pragma mark Calcul Spline on vector Y

std::vector<t_matrix> do_vec_gamma(const std::vector<t_matrix>& vec_Y, const std::vector<t_reduceTime>& vec_H, const std::pair<Matrix2D, DiagonalMatrixLD>& decomp)
{
    size_t N = vec_Y.size();

    std::vector<t_matrix> vecQtY;
    vecQtY.push_back(0.0);
    for (size_t i = 1; i < N-1; ++i) {
        const t_matrix term1 = (vec_Y[i+1] - vec_Y[i]) / vec_H[i];
        const t_matrix term2 = (vec_Y[i] - vec_Y[i-1]) / vec_H[i-1];
        vecQtY.push_back(term1 - term2);
    }
    vecQtY.push_back(0.0);

    return resolutionSystemeLineaireCholesky(decomp, vecQtY);
}

// Calcul de la matrice d'influence A
Matrix2D calculateInfluenceMatrix(double L, const DiagonalMatrixLD& WInv, const Matrix2D& Q) {
    int n = WInv.rows();

    // Créer la matrice identité
    Matrix2D I(n, true);

    // Calculer W^(-1)
    //Matrix2D WInv = createWInverse(W);

    // Calculer L * W^(-1) * Q
    auto temp = WInv * Q;
    auto LWInvQ = temp * L;

    // Calculer I + L * W^(-1) * Q
    Matrix2D sum = I + LWInvQ;

    // Calculer A = (I + L * W^(-1) * Q)^(-1)
    Matrix2D A =  sum.inverse();//  inverse(sum);

    return A;
}

// Calcul du vecteur g = Y - lambda * W-1 * Q * gamma
// tout est calculé en t_matrix (càd long double)
// Si lambda =0, il est préférable de faire directement vec_G=vec_Y
std::vector<t_matrix> do_vec_G(const SplineMatrices& matrices, const std::vector<t_matrix> &vec_Gamma, const std::vector<t_matrix>& vec_Y, const double lambdaSpline)
{
    const size_t n = vec_Y.size();
    std::vector<t_matrix> vec_G;
    vec_G.reserve(n);
    if (lambdaSpline != 0) {
        const std::vector<t_matrix> Q_gamma = multiMatParVec(matrices.matQ, vec_Gamma, 3);
        const auto& W_Inv = matrices.diagWInv.diagonal();
        const t_matrix lambda = lambdaSpline;

        for (unsigned i = 0; i < n; ++i) {
            t_matrix ldt = lambda * W_Inv[i] * Q_gamma[i];
            t_matrix g = vec_Y[i] - ldt;

            if (std::isnan(g)) {
                // qDebug()<< " isnan(g)";
                vec_G.push_back( +INFINITY) ;
            } else {
                vec_G.push_back( g) ;
            }

        }
        // ici calcul de A


    } else {
        vec_G = vec_Y;
    }

    return vec_G;
}

std::vector<double> calcul_spline_variance(const SplineMatrices& matrices, const std::pair<Matrix2D, DiagonalMatrixLD>& decomp, const double lambda)
{
    const DiagonalMatrixLD matA = diagonal_influence_matrix(matrices, 1, decomp, lambda);
    std::vector<double> varG;
    const auto& W_Inv = matrices.diagWInv.diagonal();
    size_t n = W_Inv.rows();

    for (size_t i = 0; i < n; ++i) {
        varG.push_back(static_cast<double>(matA.diagonal()[i]  * W_Inv[i]));
    }

    return varG;
}

/**
 * @brief composante_to_curve, used for fitPlot()
 * @param spline_compo
 * @param tmin
 * @param tmax
 * @param step
 * @return
 */
std::vector<QMap<double, double>> composante_to_curve(MCMCSplineComposante spline_compo, double tmin, double tmax, double step)
{
    QMap<double, double> curve;
    QMap<double, double> curve_plus;
    QMap<double, double> curve_moins;
    double g = 0.;
    double varG = 0.;
    double gp = 0.;
    double gs = 0.;
    int nb_pts = (tmax-tmin)/step + 1;
    unsigned i0 = 0;
    for (int i= 0; i < nb_pts ; ++i) {
        const double t = static_cast<double>(i) * step + tmin ;
        valeurs_G_VarG_GP_GS(t, spline_compo, g, varG, gp, gs, i0, tmin, tmax);
        curve[t] = g;
        curve_plus[t] = g + 1.96 * sqrt(varG);
        curve_moins[t] = g - 1.96 * sqrt(varG);
    }
    return std::vector<QMap<double, double>>({curve, curve_plus, curve_moins});
}


/**
 * @brief MCMCLoopCurve::doSpline
 * Fabrication de spline avec
 *      spline.vecG
 *      spline.vecGamma
 *
 * @param matrices
 * @param events
 * @param vecH
 * @param decomp
 * @param lambdaSpline
 *
 * @return SplineResults
 */
SplineResults do_spline(const std::vector<t_matrix>& vec_Y, const SplineMatrices& matrices, const std::vector<t_reduceTime>& vecH, const std::pair<Matrix2D, DiagonalMatrixLD> &decomp, const double lambdaSpline)
{
    SplineResults spline;
    try {
        // calcul de: B = R + alpha * Qt * W-1 * Q

        // Calcul du vecteur Gamma
        const std::vector<t_matrix>& vec_gamma_L = do_vec_gamma(vec_Y, vecH, decomp);

        // calcul de vecG
        if (lambdaSpline != 0.0) {
            std::vector<t_matrix> vec_G_L = do_vec_G(matrices, vec_gamma_L, vec_Y, lambdaSpline);
            spline.vecG = std::vector<double>(vec_G_L.begin(), vec_G_L.end());

        } else {
            spline.vecG = std::vector<double> (vec_Y.begin(), vec_Y.end());//vec_Y;
        }

        spline.vecGamma = std::vector<double>(vec_gamma_L.begin(), vec_gamma_L.end());


    } catch(...) {
        qCritical() << "[do_Spline] : Caught Exception!\n";
    }

    return spline;
}

// === Kernel spline cubique ===
double cubic_kernel(double x, double xi) {
    return 0.5 * std::pow(std::abs(x - xi), 3);
}

// produit des noyaux 1D (anisotrope)
// conserve la forme cubique 1D par dimension et produit → plus flexible si les axes ont des échelles différentes.
double cubic_kernel_2D(const double& at, const double& ax,
                       const double& bt, const double& bx,
                       double scale_t, double scale_x)
{
    auto k1 = cubic_kernel(at / scale_t, bt/ scale_t);
    auto k2 = cubic_kernel(ax / scale_x, bx / scale_x);
    return k1 * k2;
}

double cubic_kernel_2D_rbf(const double& ax, const double& ay,
                       const double& bx, const double& by)
{
    double dx = ax - bx;
    double dy = ay - by;
    double r = std::sqrt(dx*dx + dy*dy);
    return std::pow(std::max(0.0, 1 - r), 3); // version cubic simple
}

// --- Noyau anisotrope produit 3D ---
double cubic_kernel_3D(const double& at, const double& ax, const double& ay,
                       const double& bt, const double& bx, const double& by,
                       double scale_t, double scale_x, double scale_y)
{
    double kt = cubic_kernel(at / scale_t, bt / scale_t);
    double kx = cubic_kernel(ax / scale_x, bx / scale_x);
    double ky = cubic_kernel(ay / scale_y, by / scale_y);
    return kt * kx * ky;

}

/**
 * @brief cubic_kernel_3D
 * @param ax
 * @param ay
 * @param az
 * @param bx
 * @param by
 * @param bz
 * @return
 * 💡 Remarque importante :
 * En 3D, la distance entre points augmente plus vite → ton paramètre d’échelle
 * dans le noyau (1 - r) devra souvent être ajusté ou remplacé par un facteur d’échelle r/h
 * pour que le noyau ait une portée adaptée.
 */
double cubic_kernel_3D_rbf(const double& ax, const double& ay, const double& az,
                       const double& bx, const double& by, const double& bz)
{
    double dx = ax - bx;
    double dy = ay - by;
    double dz = az - bz;
    double r = std::sqrt(dx*dx + dy*dy + dz*dz);
    return std::pow(std::max(0.0, 1 - r), 3); // version cubic simple
}

// produit des noyaux 1D (anisotrope)
// conserve la forme cubique 1D par dimension et produit → plus flexible si les axes ont des échelles différentes.
double cubic_kernel_4D(const double& at, const double& ax, const double& ay, const double& az,
                       const double& bt, const double& bx, const double& by, const double& bz)
{
    auto k1 = cubic_kernel(at, bt);
    auto k2 = cubic_kernel(ax, bx);
    auto k3 = cubic_kernel(ay, by);
    auto k4 = cubic_kernel(az, bz);
    return k1 * k2 * k3 * k4;
}

// Radial Basis Function,
// Noyau radial basé sur la distance euclidienne (isotrope)
// dépend seulement de la distance globale → plus simple et isotrope, mais sensible à l’échelle de chaque dimension.

double cubic_kernel_4D_rbf(const double& at, const double& ax, const double& ay, const double& az,
                         const double& bt, const double& bx, const double& by, const double& bz, double h=1.0)
{
    double r = sqrt(
                   pow(at - bt,2) +
                   pow(ax - bx,2) +
                   pow(ay - by,2) +
                   pow(az - bz,2)
                   ) / h;
    if (r < 1.0) return 1 - 3*r*r + 2*r*r*r;
    else return 0;
}
// === f''(x) = sum_i 3 c_i |x - x_i| ===
//  cas pour un lissage par splines cubiques naturelles dans l’espace des distances.
double second_derivative(double x, const std::vector<double>& xi, const Eigen::Matrix<t_matrix, Dynamic, 1>& c) {
    double result = 0.0;
    for (size_t i = 0; i < xi.size(); ++i) {
        result += 6.0 * c[i] * std::abs(x - xi[i]);
    }
    return result;
}

// === Évalue f(x) = sum_i c_i * K(x, x_i) ===
double evaluate(double x, const std::vector<double>& xi, const Eigen::Matrix<t_matrix, Dynamic, 1>& c) {
    double result = 0.0;
    for (size_t i = 0; i < xi.size(); ++i) {
        result += c[i] * cubic_kernel(x, xi[i]);
    }
    return result;
}

// === Produit matriciel K * c ===
std::vector<double> apply_kernel_matrix(const std::vector<std::vector<double>>& K, const std::vector<double>& c) {
    size_t n = K.size();
    std::vector<double> result(n, 0.0);
    for (size_t i = 0; i < n; ++i)
        for (size_t j = 0; j < n; ++j)
            result[i] += K[i][j] * c[j];
    return result;
}

// === Perte quadratique avec régularisation ===
double loss(const std::vector<double>& y, const std::vector<std::vector<double>>& K, const std::vector<double>& c, double lambda) {
    size_t n = y.size();
    std::vector<double> Kc = apply_kernel_matrix(K, c);
    double loss_val = 0.0;
    for (size_t i = 0; i < n; ++i)
        loss_val += std::pow(y[i] - Kc[i], 2);
    // régularisation
    double reg = 0.0;
    for (size_t i = 0; i < n; ++i)
        for (size_t j = 0; j < n; ++j)
            reg += c[i] * K[i][j] * c[j];
    return loss_val + lambda * reg;
}

// === Gradient de la fonction de coût ===
std::vector<double> compute_gradient(const std::vector<double>& y, const std::vector<std::vector<double>>& K, const std::vector<double>& c, double lambda) {
    size_t n = y.size();
    std::vector<double> Kc = apply_kernel_matrix(K, c);
    std::vector<double> grad(n, 0.0);
    for (size_t j = 0; j < n; ++j) {
        for (size_t i = 0; i < n; ++i) {
            grad[j] += -2.0 * K[i][j] * (y[i] - Kc[i]);
        }
        for (size_t k = 0; k < n; ++k) {
            grad[j] += 2.0 * lambda * K[j][k] * c[k];
        }
    }
    return grad;
}

// === Gradient descent simple avec projection positive ===
void constrained_gradient_descent(std::vector<double>& c, const std::vector<double>& y, const std::vector<std::vector<double>>& K, double lambda, double lr = 1e-3, int max_iter = 10000) {
    for (int iter = 0; iter < max_iter; ++iter) {
        auto grad = compute_gradient(y, K, c, lambda);
        for (size_t i = 0; i < c.size(); ++i) {
            c[i] -= lr * grad[i];
            if (c[i] < 0.0) c[i] = 0.0; // projection sur c_i >= 0
        }
    }
}

// Produit scalaire
double dot(const std::vector<double>& a, const std::vector<double>& b) {
    double result = 0.0;
    for (size_t i = 0; i < a.size(); ++i)
        result += a[i] * b[i];
    return result;
}

// Produit matrice * vecteur
std::vector<double> mat_vec(const Matrix2D& M, const std::vector<double>& v) {
    std::vector<double> result(M.size(), 0.0);
    for (long i = 0; i < M.rows(); ++i)
        for (long j = 0; j < M.cols(); ++j)
            result[i] += M(i, j) * v[j];
    return result;
}

// Inversion naive d'une matrice symétrique positive définie via Gauss-Jordan
std::vector<std::vector<double>> inverse(const std::vector<std::vector<double>>& A) {
    size_t n = A.size();
    std::vector<std::vector<double>> inv(n, std::vector<double>(n, 0.0));
    std::vector<std::vector<double>> aug(A);

    // Create identity matrix
    for (size_t i = 0; i < n; ++i)
        inv[i][i] = 1.0;

    // Gauss-Jordan elimination
    for (size_t i = 0; i < n; ++i) {
        double pivot = aug[i][i];
        for (size_t j = 0; j < n; ++j) {
            aug[i][j] /= pivot;
            inv[i][j] /= pivot;
        }
        for (size_t k = 0; k < n; ++k) {
            if (k == i) continue;
            double factor = aug[k][i];
            for (size_t j = 0; j < n; ++j) {
                aug[k][j] -= factor * aug[i][j];
                inv[k][j] -= factor * inv[i][j];
            }
        }
    }

    return inv;
}

// Calcule la variance autour de f(x0)
double prediction_variance(
    t_matrix x0,
    const std::vector<double>& xi,
    const Matrix2D& K,
    const std::vector<double>& W_inv, // W^-1 diagonal
    t_matrix lambda,
    t_matrix sigma2
) {
    size_t n = xi.size();

    // Construire M = K + lambda * W^-1
    Matrix2D M = K;
    for (size_t i = 0; i < n; ++i)
        M(i, i) += lambda * W_inv[i];

    Eigen::LDLT<Matrix2D> solver(M);

    // Kx = [K(x0, x1), ..., K(x0, xn)]
    Eigen::Matrix<t_matrix, Dynamic, 1> Kx(n);
    for (size_t i = 0; i < n; ++i)
        Kx(i) = cubic_kernel(x0, xi[i]);

    // tmp = W * solver.solve(Kx)   (ici W = diag(1 / W_inv))
    Eigen::Matrix<t_matrix, Dynamic, 1> tmp = solver.solve(Kx);
    for (size_t i = 0; i < n; ++i)
        tmp(i) /= W_inv[i]; // W = diag(1 / W_inv)

    // M_inv_W_solver_Kx
    Eigen::Matrix<t_matrix, Dynamic, 1> M_inv_Kx = solver.solve(tmp);

    return sigma2 * Kx.dot(M_inv_Kx);
}



// Résolution d'un système linéaire M * x = b (Gauss-Jordan)
std::vector<double> solveLinearSystem(std::vector<std::vector<double>> M, std::vector<double> b)
{
    size_t n = M.size();
    for (size_t i = 0; i < n; ++i) {
        double pivot = M[i][i];
        for (size_t j = 0; j < n; ++j)
            M[i][j] /= pivot;
        b[i] /= pivot;
        for (size_t k = 0; k < n; ++k) {
            if (k != i) {
                double factor = M[k][i];
                for (size_t j = 0; j < n; ++j)
                    M[k][j] -= factor * M[i][j];
                b[k] -= factor * b[i];
            }
        }
    }
    return b;
}

// Fonction de tri
void sort_by_x(std::vector<t_matrix>& x, std::vector<t_matrix>& y, std::vector<t_matrix>& w) {
    struct DataPoint {
        t_matrix x;
        t_matrix y;
        t_matrix w; // facultatif, utile si tu veux trier aussi les poids
    };

    size_t n = x.size();
    std::vector<DataPoint> data(n);

    // Rassembler les données
    for (size_t i = 0; i < n; ++i)
        data[i] = {x[i], y[i], w[i]};

    // Trier par x croissant
    std::sort(data.begin(), data.end(), [](const DataPoint& a, const DataPoint& b) {
        return a.x < b.x;
    });

    // Réassigner les valeurs triées
    for (size_t i = 0; i < n; ++i) {
        x[i] = data[i].x;
        y[i] = data[i].y;
        w[i] = data[i].w;
    }
}


// Variance de chaque point lissé : Var[ŷ_i] = σ² * ∑_j S_ij²
std::vector<t_matrix> compute_pointwise_variance(
    const Matrix2D& S,
    t_matrix sigma2
    )
{
    size_t n = S.rows();
    std::vector<t_matrix> var_yhat(n, 0.0);

    for (size_t i = 0; i < n; ++i) {
        t_matrix sum_sq = 0.0;
        for (size_t j = 0; j < n; ++j) {
            t_matrix val = S(i, j);
            sum_sq += val * val;
        }
        var_yhat[i] = sigma2 * sum_sq;
    }
    return var_yhat;
}



// --- cubic kernel 2D : |t - t_i|^3 ---
// --- notre cubic kernel 2D : 1/2*|t - t_i|^3 ---
double cubic_kernel_2D_second_derivative(double t, double ti)
{
    // dérivée seconde de (1 - |t - ti|)^3 = 6 * |t - ti|
    return 3.0 * abs(t - ti); // tu peux adapter si ton noyau est différent
}

// --- Calcul des dérivées secondes aux nœuds ---
Matrix2D second_derivative_t_nodes(
    const std::vector<double>& t_nodes,
    const Matrix2D& C // n x m, m composantes
    )
{
    size_t n = t_nodes.size();
    size_t m = C.cols();
    Matrix2D Fpp(n, m); // matrice résultat

    for (size_t j = 0; j < n; ++j) {          // pour chaque noeud t_j
        for (size_t k = 0; k < m; ++k) {      // pour chaque composante
            double s = 0.0;
            for (size_t i = 0; i < n; ++i) {  // somme sur les coefficients
                s += C(i, k) * cubic_kernel_2D_second_derivative(t_nodes[j], t_nodes[i]);
            }
            Fpp(j, k) = s;
        }
    }
    return Fpp;
}

double compute_GCV_weighted(
    const Matrix2D& Y_mat,        // valeurs observées (n × d)
    const Matrix2D& Y_hat,        // valeurs prédites (n × d)
    const ColumnVectorLD& W_1,       // diagonale de W_1 (taille n)
    const Matrix2D& S             // matrice d'influence A (n × n)
    )
{
    size_t n_points = Y_mat.rows();
    size_t n_components = Y_mat.cols();

    // Trace(A)
    double traceA = S.trace();

    // Résidu pondéré
    double rss_weighted = 0.0;
    for (size_t i = 0; i < n_points; ++i) {
        for (size_t j = 0; j < n_components; ++j) {
            double err = Y_mat(i, j) - Y_hat(i, j);
            rss_weighted += (err * err) * W_1[i];
        }
    }

    double denom = n_points * (1.0 - traceA / n_points);
    double gcv = rss_weighted / (denom * denom);

    return gcv;
}

std::pair<MCMCSpline, std::pair<double, double>> do_spline_kernel_composante(const std::vector<double> &vec_t, const std::vector<double> &vec_X, const std::vector<double> &vec_X_err, double tmin, double tmax, SilvermanParam &sv, const std::vector<double> &vec_Y, const std::vector<double> &vec_Y_err, const std::vector<double> &vec_Z, const std::vector<double> &vec_Z_err)
{
    bool doY = (!vec_Y.empty() && vec_Z.empty());
    bool doYZ = (!vec_Y.empty() && !vec_Z.empty());

    std::vector<t_matrix> vec_tmp_t;
    std::vector<t_matrix> vec_tmp_x, vec_tmp_y, vec_tmp_z;
    std::vector<t_matrix> vec_tmp_x_err, vec_tmp_y_err, vec_tmp_z_err;
    // trie des temps et des données associées
    if (!std::is_sorted(vec_t.begin(), vec_t.end())) {

        std::vector<size_t> l_index = argsort(vec_t);

        for (size_t i : l_index) {
            vec_tmp_t.push_back(vec_t.at(i));
            vec_tmp_x.push_back(vec_X.at(i));
            vec_tmp_x_err.push_back(vec_X_err.at(i));
            if (doY || doYZ) {
                vec_tmp_y.push_back(vec_Y.at(i));
                vec_tmp_y_err.push_back(vec_Y_err.at(i));
            }
            if (doYZ) {
                vec_tmp_z.push_back(vec_Z.at(i));
                vec_tmp_z_err.push_back(vec_Z_err.at(i));
            }
        }
    } else {
        vec_tmp_t = std::vector<t_matrix>(vec_t.begin(), vec_t.end());
        vec_tmp_x = std::vector<t_matrix>(vec_X.begin(), vec_X.end());
        vec_tmp_x_err = std::vector<t_matrix>(vec_X_err.begin(), vec_X_err.end());
        if (doY || doYZ) {
            vec_tmp_y = std::vector<t_matrix>(vec_Y.begin(), vec_Y.end());
            vec_tmp_y_err = std::vector<t_matrix>(vec_Y_err.begin(), vec_Y_err.end());
        }
        if (doYZ) {
            vec_tmp_z = std::vector<t_matrix>(vec_Z.begin(), vec_Z.end());
            vec_tmp_z_err = std::vector<t_matrix>(vec_Z_err.begin(), vec_Z_err.end());
        }
    }

    std::vector<double> W_1;
    if (sv.use_error_measure) {
        if (doY) {
            auto Y_err = vec_tmp_y_err.begin();
            for (auto X_err : vec_tmp_x_err) {
                const t_matrix Sy = pow(X_err, -2.) + pow(*Y_err++, -2.) ;
                W_1.push_back(2./Sy);
            }
        } else if (doYZ) {
            auto Y_err = vec_tmp_y_err.begin();
            auto Z_err = vec_tmp_z_err.begin();
            for (auto X_err : vec_tmp_x_err) {
                const t_matrix Sy = pow(X_err, -2.) + pow(*Y_err++, -2.) + pow(*Z_err++, -2.) ;
                W_1.push_back(3./Sy);
            }
        } else {

            for (auto X_err : vec_tmp_x_err) {
                W_1.push_back(pow(X_err, 2.));
            }
        }

    } else {
        W_1 = std::vector<double>(vec_tmp_x_err.size(), 1.0);
    }

    size_t n = vec_tmp_t.size();
    std::vector<t_reduceTime> vec_theta_red;

    for (auto t : vec_tmp_t) {
        vec_theta_red.push_back((t-tmin)/(tmax-tmin));
    }
    // --kernel method
    // === Données ===
    std::vector<double> t = vec_theta_red;//{0.0, 1.0, 2.0, 3.0};

    std::vector<t_matrix> x = vec_tmp_x;// vec_X;//{1.0, 2.0, 1.5, 3.0};

    std::vector<t_matrix> y = vec_tmp_y;
    std::vector<t_matrix> z = vec_tmp_z;

    // Construction du vecteur x
    ColumnVectorLD x_vec, y_vec, z_vec;
    Matrix2D Y_mat(n, 1);

    x_vec.resize(n);
    for (size_t i = 0; i < n; ++i)
        x_vec(i) = x[i];

    Y_mat << x_vec;

    if (doY) {
        Y_mat.resize(n, 2);
        y_vec.resize(n);
        for (size_t i = 0; i < n; ++i)
            y_vec(i) = y[i];

        Y_mat << x_vec, y_vec;

    } else if (doYZ) {
        Y_mat.resize(n, 3);
        y_vec.resize(n);
        z_vec.resize(n);
        for (size_t i = 0; i < n; ++i) {
            y_vec(i) = y[i];
            z_vec(i) = z[i];
        }

        Y_mat << x_vec, y_vec, z_vec;
    }

    // Matrice K symétrique
    // Typiquement, pour lisser X(t),Y(t),Z(t) on construit K sur t uniquement
    // et on met les sorties dans Y ∈ R^(n×3).
    Matrix2D K (n, n);
    for (size_t i = 0; i < n; ++i)
        for (size_t j = 0; j < n; ++j)
            K(i, j) = cubic_kernel(t[i], t[j]);

    // -------------------------------
    // Balayage de lambda et GCV
    // -------------------------------
    std::vector<double> lambdas;        // les λ testés
    std::vector<long double> GCV_vals;  // valeurs brutes GCV
    std::vector<long double> CV_vals;   // valeurs brutes CV

    double best_lambda_rss= 0.0, best_lambda_cv = 0.0, best_lambda_gcv = 0.0;

    double best_rss = std::numeric_limits<double>::infinity();
    double best_cv = std::numeric_limits<double>::infinity();
    double best_gcv = std::numeric_limits<double>::infinity();

    // M = K + lambda * W^-1
    // M * C = Y
    // S = K * M^{-1}
    // Y_hat = S * Y
    Matrix2D M, C, S, Y_hat;
    double lambda;

    for (int log_lambda = -1000; log_lambda < 1001; log_lambda +=5) {
        lambda = std::pow(10.0, log_lambda/100.0);

        // M = K + lambda * W^-1 (ici W_1 est déjà W^-1 diag)
        M = K;
        for (size_t i = 0; i < n; ++i)
            M(i, i) += lambda * W_1[i];

        // Résolution : M * C = Y
        C = M.ldlt().solve(Y_mat);

        // S = K * M^{-1}
        S = K * M.ldlt().solve(Matrix2D::Identity(n, n));

        // y_hat = S * y // S est la matrice d'influence aussi appelé A
        Y_hat = S * Y_mat;

        // Trace(S)
        auto traceS = S.trace();

        // GCV
        size_t n_points = Y_mat.rows();
        size_t n_components = Y_mat.cols();

        // =====================
        // 1️⃣ RSS (non pondéré)
        // =====================
        double rss = (Y_mat - Y_hat).squaredNorm();
        if (rss < best_rss) {
            best_rss = rss;
            best_lambda_rss = lambda;
        }

        // =====================
        // 2️⃣ GCV pondéré
        // =====================
        double weighted_rss = 0.0;
        for (size_t i = 0; i < n_points; ++i) {
            double w_ii = 1.0 / W_1[i]; // récupérer W_{ii} (car W_1 = W^{-1})
            for (size_t j = 0; j < n_components; ++j) {
                double res = Y_mat(i, j) - Y_hat(i, j);
                weighted_rss += (res * res) / w_ii;
            }
        }
        double dle   = 1.0 - (traceS / n_points); // degrees of freedom correction
        double gcv   = (weighted_rss / (n_points * n_components)) / (dle * dle);
        if (gcv < best_gcv) {
            best_gcv = gcv;
            best_lambda_gcv = lambda;
        }

        // =====================
        // 3️⃣ CV pondéré (LOOCV)
        // =====================
        double cv = 0.0;
        for (size_t i = 0; i < n_points; ++i) {
            double denom = 1.0 - S(i, i);
            double w_ii  = 1.0 / W_1[i]; // W_{ii}
            for (size_t j = 0; j < n_components; ++j) {
                double err = (Y_mat(i, j) - Y_hat(i, j)) / denom;
                cv += (err * err) / w_ii;
            }
        }
        cv /= (n_points * n_components);
        if (cv < best_cv) {
            best_cv = cv;
            best_lambda_cv = lambda;
        }


        // Sauvegarde dans vecteurs + tables
        lambdas.push_back(lambda);
        GCV_vals.push_back(gcv);
        CV_vals.push_back(cv);

        sv.tab_GCV[lambda] = gcv;
        sv.tab_CV[lambda]   = cv;
    }



    // -------- Calcul avec resultat du noyaux
    std::cout << "[do_spline_kernel_composante] best Lambda (RSS) = " << best_lambda_rss
        << "   RSS min = " << best_rss << std::endl;
    std::cout << "[do_spline_kernel_composante] best Lambda (GCV) = " << best_lambda_gcv
              << "   GCV min = " << best_gcv << std::endl;
    std::cout << "[do_spline_kernel_composante] best Lambda (CV)  = " << best_lambda_cv
              << "   CV min  = " << best_cv << std::endl;


    // --------------------------------------------------
    // Filtrage + recherche du min pour GCV et CV
    // --------------------------------------------------
    auto process_min = [&](std::vector<long double> vals) {
        // Supprimer zéros initiaux
        auto it = std::find_if(vals.begin(), vals.end(),
                               [](long double v) { return v != 0.0; });
        size_t nb_0 = std::distance(vals.begin(), it);
        if (it != vals.begin())
            vals.erase(vals.begin(), it);

        // Filtre gaussien
        //vals = gaussian_filter(vals, 10.0, 1);
        constexpr int padding_type = 1;
        double sigma_filter= vals.size() / 20.0;
        vals = gaussian_filter(vals, sigma_filter, padding_type);

        // Recherche min
        size_t idx_min = std::distance(vals.begin(),
                                       std::min_element(vals.begin(), vals.end()));
        idx_min += nb_0; // correction index après suppression zéros

        return idx_min;
    };

    size_t idx_gcv = process_min(GCV_vals);
    size_t idx_cv  = process_min(CV_vals);
    best_lambda_gcv = lambdas[idx_gcv];
    best_lambda_cv  = lambdas[idx_cv];

    std::cout << "[do_spline_kernel_composante]"
             << "Lambda optimal (GCV) =" << best_lambda_gcv
             << " GCV min =" << GCV_vals[idx_gcv] << std::endl;

    std::cout << "[do_spline_kernel_composante]"
             << "Lambda optimal (CV)  =" << best_lambda_cv
              << " CV min  =" << CV_vals[idx_cv] << std::endl;


    double lambda_mini;

    if (idx_gcv > 0 && idx_gcv < static_cast<size_t>(lambdas.size() - 1)) {
        // Solution évidente avec GCV
        lambda_mini = best_lambda_gcv;
        sv.comment = "GCV solution; ";

    } else if (idx_cv > 0 && idx_cv < static_cast<size_t>(lambdas.size() - 1)) {
        // Solution évidente avec CV
        lambda_mini = best_lambda_cv;
        sv.comment = "CV solution; ";

    } else {
        // Si pas de minimum clair
        if (GCV_vals.front() > GCV_vals.back()) {
            lambda_mini = 1.0e20;  // Lin. regression
            sv.comment = "No GCV solution; Linear Regression; ";
        } else {
            lambda_mini = 0.0;     // Interpolation spline
            sv.comment = "No GCV solution; Spline Interpolation; ";
        }
    }

    //  _________________
    // ex:cross_validation lambda :  0.0040738  = 10E -2.39  Vg= 3.12168 sqrt(Vg) 1.76683
    lambda = lambda_mini; //0.0040738;//pow(10, -2.39);
    sv.log_lambda_value = log10(lambda);
    // Construction de M = K + lambda * W^-1
    M = K; // K ne dépend que des temps
    for (size_t i = 0; i < n; ++i)
        M(i, i) += lambda * W_1[i];

    // === Optimisation ===
    // pour contraindre la positivité
   // constrained_gradient_descent(c, y, M, lambda);

    // === Affichage des résultats ===


    // => On le calcule ici pour la première composante (x)

    //double lambda_cv = best_lambda_cv; //pow(10., sv.log_lambda_value);
    //double Vg;


    // Reconstruction avec le lambda optimal GCV// Construction de M = K + lambda * W^-1
    Matrix2D M_best = K;
    for (size_t i = 0; i < n; ++i)
        M_best(i, i) += lambda * W_1[i];

    auto solver_best = M_best.ldlt();

    // Matrice de lissage S_best
    Matrix2D S_best = K * solver_best.solve(Matrix2D::Identity(n, n));


    // Courbe lissée Y_hat contient toutes les composantes, rangées par colonne
    Y_hat = S_best * Y_mat;
    size_t n_points = Y_hat.rows();
    size_t n_components = Y_hat.cols();

    // 1️⃣ Variance globale des valeurs lissées
    double mean_Y_hat = Y_hat.mean();
    double V_global = (Y_hat.array() - mean_Y_hat).square().sum() / (n_points * n_components);

    // 2️⃣ Variance résiduelle corrigée (Vg = sigma²)
    double traceS = S_best.trace();

    /* Note : certaines références utilisent traceS ou traceS * n_components selon que S est la matrice de lissage pour chaque composante ou la même pour toutes.
     *  Donc si tu appliques le même S_best à toutes les colonnes, tu pourrais multiplier traceS par n_components :
     */
    //double sigma2 = (Y_mat - Y_hat).squaredNorm() / (n_points * n_components - traceS * n_components);

    double EDF = n_points - traceS;
    double sigma2 = 0.0;
    for (int j = 0; j < Y_mat.cols(); ++j) {
        Eigen::Vector<t_matrix, Dynamic> residual = Y_mat.col(j) - Y_hat.col(j);
        sigma2 += residual.squaredNorm() / EDF;
    }
    sigma2 /= Y_mat.cols(); // moyenne sur les composantes


    std::cout << "[do_spline_kernel_composante] Variance globale des valeurs lissées (Vg) = " << V_global << "\n";
    std::cout << "[do_spline_kernel_composante] Variance résiduelle corrigée (sigma²)     = " << sigma2 << "\n";
// ---

    //--
    double Vg = sigma2;

    std::pair<double, double> lambda_Vg = std::make_pair(lambda, Vg);

    //  Spline final

    // Résolution du système M * C = Y  pour toutes les composantes
    C = M.ldlt().solve(Y_mat);
    auto Ydt2 = second_derivative_t_nodes(t, C);
    // Résolution : M * ci = yi
    ColumnVectorLD ci = M.ldlt().solve(x_vec);  // LDLT pour matrice symétrique

    std::vector<double> vec_G, vec_gamma, vec_varG;
    // S_best est la matrice d'influence A
    std::vector<t_matrix> var_pointwise  = compute_pointwise_variance(S_best, sigma2);
    vec_varG = std::vector<double>(var_pointwise.begin(), var_pointwise.end());

    vec_G = std::vector<double>(Y_hat.col(0).begin(), Y_hat.col(0).end());
    vec_gamma = std::vector<double>(Ydt2.col(0).begin(), Ydt2.col(0).end());


    // --------------------------------------------------------------
    //  Calcul de la spline g, g" pour chaque composante x y z + stockage
    // --------------------------------------------------------------
    MCMCSpline spline;

    MCMCSplineComposante splineX, splineY, splineZ;

    splineX.vecThetaReduced = vec_theta_red;
    splineX.vecG = vec_G;
    splineX.vecGamma = vec_gamma;

    splineX.vecVarG = vec_varG;

    if (doY) {

        vec_G = std::vector<double>(Y_hat.col(1).begin(), Y_hat.col(1).end());
        vec_gamma = std::vector<double>(Ydt2.col(1).begin(), Ydt2.col(1).end());
        std::vector<t_matrix> var_pointwise  = compute_pointwise_variance(S_best, sigma2);

        vec_varG = std::vector<double>(var_pointwise.begin(), var_pointwise.end());

        splineY.vecThetaReduced = vec_theta_red;
        splineY.vecG = vec_G;
        splineY.vecGamma = vec_gamma;

        splineY.vecVarG = vec_varG;

    } else if (doYZ) {

        vec_G = std::vector<double>(Y_hat.col(1).begin(), Y_hat.col(1).end());
        vec_gamma = std::vector<double>(Ydt2.col(1).begin(), Ydt2.col(1).end());

        std::vector<t_matrix> var_pointwise  = compute_pointwise_variance(S_best, sigma2);
        vec_varG = std::vector<double>(var_pointwise.begin(), var_pointwise.end());


        splineY.vecThetaReduced = vec_theta_red;
        splineY.vecG = vec_G;
        splineY.vecGamma = vec_gamma;

        splineY.vecVarG = vec_varG;

        // Z

        vec_G = std::vector<double>(Y_hat.col(2).begin(), Y_hat.col(2).end());
        vec_gamma = std::vector<double>(Ydt2.col(2).begin(), Ydt2.col(2).end());

        var_pointwise  = compute_pointwise_variance(S_best, sigma2);
        vec_varG = std::vector<double>(var_pointwise.begin(), var_pointwise.end());

        splineZ.vecThetaReduced = vec_theta_red;
        splineZ.vecG = vec_G;
        splineZ.vecGamma = vec_gamma;

        splineZ.vecVarG = vec_varG;

    }

    spline.splineX = std::move(splineX);
    spline.splineY = std::move(splineY);
    spline.splineZ = std::move(splineZ);
    return std::make_pair( spline, lambda_Vg);

}

std::pair<MCMCSpline, std::pair<double, double>> do_spline_composante(const std::vector<double> &vec_t, const std::vector<double> &vec_X, const std::vector<double> &vec_X_err, double tmin, double tmax, SilvermanParam& sv, const std::vector<double> &vec_Y, const std::vector<double> &vec_Y_err, const std::vector<double> &vec_Z, const std::vector<double> &vec_Z_err)
{

    bool doY (!vec_Y.empty() && vec_Z.empty());
    bool doYZ (!vec_Y.empty() && !vec_Z.empty());

    std::vector<t_matrix> vec_tmp_t;
    std::vector<t_matrix> vec_tmp_x, vec_tmp_y, vec_tmp_z;
    std::vector<t_matrix> vec_tmp_x_err, vec_tmp_y_err, vec_tmp_z_err;
    // trie des temps et des données associées
    if (!std::is_sorted(vec_t.begin(), vec_t.end())) {
        std::vector<int> l_index = get_order(vec_t);

        for (int i : l_index) {
            vec_tmp_t.push_back(vec_t.at(i));
            vec_tmp_x.push_back(vec_X.at(i));
            vec_tmp_x_err.push_back(vec_X_err.at(i));
            if (doY || doYZ) {
                vec_tmp_y.push_back(vec_Y.at(i));
                vec_tmp_y_err.push_back(vec_Y_err.at(i));
            }
            if (doYZ) {
                vec_tmp_z.push_back(vec_Z.at(i));
                vec_tmp_z_err.push_back(vec_Z_err.at(i));
            }
        }
    } else {
        vec_tmp_t = std::vector<t_matrix>(vec_t.begin(), vec_t.end()); //vec_t;
        vec_tmp_x = std::vector<t_matrix>(vec_X.begin(), vec_X.end()); //vec_X;
        vec_tmp_x_err = std::vector<t_matrix>(vec_X_err.begin(), vec_X_err.end()); //vec_X_err;
        if (doY || doYZ) {
            vec_tmp_y = std::vector<t_matrix>(vec_Y.begin(), vec_Y.end());
            vec_tmp_y_err = std::vector<t_matrix>(vec_Y_err.begin(), vec_Y_err.end()); //vec_Y_err;
        }
        if (doYZ) {
            vec_tmp_z = std::vector<t_matrix>(vec_Z.begin(), vec_Z.end()); //vec_Z;
            vec_tmp_z_err = std::vector<t_matrix>(vec_Z_err.begin(), vec_Z_err.end()); //vec_Z_err;
        }
    }

    std::vector<t_reduceTime> vecH;
    std::vector<t_reduceTime> vec_theta_red;

    for (auto t : vec_tmp_t) {
        vec_theta_red.push_back((t-tmin)/(tmax-tmin));
        //qDebug()<<"do spline compo t="<< t;
    }

    spread_theta_reduced(vec_theta_red);

    auto iter = vec_theta_red.begin();
    for (; iter != std::prev(vec_theta_red.end()) ; iter++) {
        vecH.push_back(*std::next(iter) -  *iter);
    }
    // doSpline utilise les Y des events
    // => On le calcule ici pour la première composante (x)

    double lambda_cv, Vg;
    std::pair<double, double> lambda_Vg;
    if (sv.lambda_process == SilvermanParam::lambda_type::Silverman) {
        lambda_Vg = initLambdaSplineBySilverman(sv, vec_tmp_x, vec_tmp_x_err, vecH, vec_tmp_y, vec_tmp_y_err, vec_tmp_z, vec_tmp_z_err);
        lambda_cv = lambda_Vg.first;

        Vg = lambda_Vg.second;

    } else {
        lambda_cv = pow(10., sv.log_lambda_value);


        DiagonalMatrixLD W_1 (vec_tmp_x_err.size());
        int i = 0;
        for (auto X_err : vec_tmp_x_err) {
            W_1.diagonal()[i++] = X_err * X_err;
        }


        const SplineMatrices& spline_matrices_tmp = prepare_calcul_spline(vecH, W_1);
        Vg = var_residual(vec_tmp_x, spline_matrices_tmp, vecH, lambda_cv);

        if (doY) {
            Vg += var_residual(vec_tmp_y, spline_matrices_tmp, vecH, lambda_cv);
            Vg /= 2.;
        } else if (doYZ) {
            Vg += var_residual(vec_tmp_y, spline_matrices_tmp, vecH, lambda_cv) + var_residual(vec_tmp_z, spline_matrices_tmp, vecH, lambda_cv);
            Vg /= 3.;
        }


    }
    lambda_Vg = std::make_pair(lambda_cv, Vg);
    qDebug()<<" end of cross_validation lambda : "<<lambda_cv<<" = 10E"<<log10(lambda_cv)<< " Vg="<< Vg <<"sqrt(Vg)"<<sqrt(Vg);

   // std::vector<t_matrix> W_1;
    DiagonalMatrixLD w_1 (vec_tmp_x_err.size());
    int i = 0;
    if (sv.use_error_measure) {
        if (doY) {
            auto Y_err = vec_tmp_y_err.begin();
            w_1.resize(vec_tmp_x_err.size());

            for (auto X_err : vec_tmp_x_err) {
                const double Sy = pow(X_err, -2.) + pow(*Y_err++, -2.) ;
                w_1.diagonal()[i++] = 2.0 / Sy;
               // W_1.push_back(2./Sy);
            }
        } else if (doYZ) {
            auto Y_err = vec_tmp_y_err.begin();
            auto Z_err = vec_tmp_z_err.begin();
            for (auto X_err : vec_tmp_x_err) {
                const double Sy = pow(X_err, -2.) + pow(*Y_err++, -2.) + pow(*Z_err++, -2.) ;
                w_1.diagonal()[i++] = 3.0 / Sy;
                //W_1.push_back(3./Sy);
            }
        } else {

            for (auto X_err : vec_tmp_x_err) {
                w_1.diagonal()[i++] = pow(X_err, 2.);
                //W_1.push_back(pow(X_err, 2.));
            }
        }

    } else {
        //W_1 = std::vector<t_matrix>(vec_tmp_x_err.size(), 1.0L);
        w_1.setIdentity(vec_tmp_x_err.size());
    }

    const SplineMatrices& spline_matrices = prepare_calcul_spline(vecH, w_1);


  //  Spline final

    std::unique_ptr<std::pair<Matrix2D, DiagonalMatrixLD>> decomp;

    if (lambda_cv != 0.0) {
        decomp = std::make_unique<std::pair<Matrix2D, DiagonalMatrixLD>>(decomp_matB(spline_matrices, lambda_cv));

    } else {
        decomp = std::make_unique<std::pair<Matrix2D, DiagonalMatrixLD>>(decompositionCholesky(spline_matrices.matR, 5, 1));
    }


    const SplineResults& sx = do_spline(vec_tmp_x, spline_matrices,  vecH, *decomp, lambda_cv);

    std::vector<double> varG;

    if (lambda_cv == 0) {
        if (sv.use_error_measure) {
            varG = std::vector<double>(w_1.diagonal().begin(), w_1.diagonal().end());

        } else {
            varG =  std::vector<double>(w_1.size(), 0.0);
        }


    } else {
        DiagonalMatrixLD matA = diagonal_influence_matrix(spline_matrices, 1, *decomp, lambda_cv);
        // Affectation de Vg pour l'affichage de l'erreur global
        matA = matA * Vg;

        varG = std::vector<double>(matA.diagonal().data(), matA.diagonal().data() + matA.diagonal().size());

        //for (auto ai : matA)
          //  varG.push_back(ai*Vg);
    }


    // Cas Erreur ré-évaluée suivant Silverman, Some aspect..., page 7
    /*
    int n = matA.size();
    std::vector<double> general_resi = general_residual(vec_X, spline_matrices, vecH, lambda_cv);

    for (int i = 0; i < n; ++i) {

        const int k = 5; // Dans l'article utilisation de la valeur 5 ?? à addapter suivant les modèles
        const int mi =  (i-k<0 ? 0: i-k);// std::max(0, i-k);
        const int ni =  (i+k<n ? i+k : n);
        double reestimat_W_1 = 0.;
        for (int j = mi; j< ni; j++) {
            reestimat_W_1 += pow(general_resi[j], 2);
        }
        reestimat_W_1 *= spline_matrices.diagWInv[i];
        reestimat_W_1 /= (ni-mi);

        varG.push_back(matA[i]  * reestimat_W_1);

    }
    */
    // --------------------------------------------------------------
    //  Calcul de la spline g, g" pour chaque composante x y z + stockage
    // --------------------------------------------------------------
    MCMCSpline spline;

    MCMCSplineComposante splineX, splineY, splineZ;

    splineX.vecThetaReduced = vec_theta_red;
    splineX.vecG = std::move(sx.vecG);
    splineX.vecGamma = std::move(sx.vecGamma);

    splineX.vecVarG = varG;

    /*showVector(splineX.vecThetaReduced, "do_spline_composante splineX.vecThetaReduced");
    showVector(splineX.vecG, "splineX.vecG");
    showVector(splineX.vecGamma, "splineX.vecGamma");
    showVector(splineX.vecVarG, "splineX.vecVarG");
*/

    if (doY || doYZ) {
        const SplineResults &sy = do_spline(vec_tmp_y, spline_matrices,  vecH, *decomp, lambda_cv);

        splineY.vecThetaReduced = vec_theta_red;
        splineY.vecG = std::move(sy.vecG);
        splineY.vecGamma = std::move(sy.vecGamma);

        splineY.vecVarG = varG;

        if (doYZ) {
            const SplineResults &sz = do_spline(vec_tmp_z, spline_matrices,  vecH, *decomp, lambda_cv);

            splineZ.vecThetaReduced = vec_theta_red;
            splineZ.vecG = std::move(sz.vecG);
            splineZ.vecGamma = std::move(sz.vecGamma);

            splineZ.vecVarG = varG;
        }
    }

    spline.splineX = std::move(splineX);
    spline.splineY = std::move(splineY);
    spline.splineZ = std::move(splineZ);

    return std::make_pair( spline, lambda_Vg);
}


long double cross_validation (const std::vector<t_matrix>& vec_Y, const SplineMatrices& matrices, const std::vector<t_reduceTime>& vecH, const double lambda)
{

    const long double N = matrices.diagWInv.rows();
    //showMatrix(matrices.matR, "matR");
    //showMatrix(matrices.matQ, "matQ");


    Matrix2D matB = addMatEtMat(matrices.matR, multiConstParMat(matrices.matQTW_1Q, lambda, 5), 5);

    //Matrix2D Btest = matrices.matR + lambda*matrices.matQTW_1Q;
    //showMatrix(Btest, "Btest");

//showMatrix(matB, "matB"); // vérifier si symetrique ------
    // Decomposition_Cholesky de matB en matL et matD
    // Si alpha global: calcul de Mat_B = R + alpha * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
    const auto &decomp = decompositionCholesky(matB, 5, 1);

    //const SplineResults& s = do_spline(vec_Y, matrices,  vecH, decomp, lambda);

    // On calcul la spline g en type t_matrix= long double pour plus de précision

    const std::vector<t_matrix>& vec_gamma_L = do_vec_gamma(vec_Y, vecH, decomp);

    const std::vector<t_matrix>& g = do_vec_G(matrices, vec_gamma_L, vec_Y, lambda);

    const DiagonalMatrixLD matA = diagonal_influence_matrix(matrices, 1, decomp, lambda);

    long double CV = 0.0;
    for (int i = 0 ; i < N; i++) {
        CV +=  pow((g[i] - vec_Y[i]) / (1.0 - matA.diagonal()[i]), 2.0)/ matrices.diagWInv.diagonal()[i];
    }

    return CV/N;
}

// Algorithme de Kahan pour somme compensée (améliore la précision)
/*static long double compensated_sum(const std::vector<double>& values) {
    long double sum = 0.0L;
    long double c = 0.0L;  // Terme de compensation

    for (const double& val : values) {
        long double y = static_cast<long double>(val) - c;
        long double t = sum + y;
        c = (t - sum) - y;
        sum = t;
    }

    return sum;
}
*/

// Algorithme de Kahan pour somme compensée (améliore la précision)
long double compensated_sum(const std::vector<long double>& values)
{
    long double sum = 0.0L;
    long double c = 0.0L;  // Terme de compensation

    for (const long double& val : values) {
        long double y = val - c;
        long double t = sum + y;
        c = (t - sum) - y;
        sum = t;
    }

    return sum;
}

/**
 * @brief Calculates the Generalized Cross-Validation (GCV) score for a spline model.
 * i.e. the prediction of $(g_(ti)-Yij)$ without the point Yij. With weight in matrices
 *
 * This function computes the GCV score, which is used to select the optimal
 * smoothing parameter (lambda) in spline regression. The GCV score is calculated
 * using the observed data (vec_Y), the estimated values (s.vecG), and the influence
 * of each data point (matA).
 *
 * The formula for GCV is:
 * \f[
 * GCV = \frac{\sum_{i=1}^{N} \frac{(Y_i - \hat{G}_i)^2}{W_{ii}}}{(N(1 - \frac{\text{trace}(A)}{N}))^2}
 * \f]
 * where:
 * - \( Y_i \) is the observed value
 * - \( \hat{G}_i \) is the predicted value
 * - \( W_{ii} \) is the diagonal element of the weight matrix (inverse of the precision)
 * - \( N \) is the number of observations
 * - \( A \) is the influence matrix
 * - \( \text{trace}(A) \) is the sum of the diagonal elements of matrix \( A \)
 * - \( DLEc = 1 - \frac{\text{trace}(A)}{N} \) is the degrees of freedom correction
 *
 * @param vec_Y Vector of observed values (dependent variable)
 * @param matrices Struct containing matrices used in spline calculations
 * @param vecH Vector related difference of time
 * @param lambda Smoothing parameter
 * @return double The computed GCV score
 */
long double general_cross_validation (const std::vector<t_matrix> &vec_Y, const SplineMatrices& matrices, const std::vector<t_reduceTime> &vecH, const double lambda)
{
    const double N = matrices.diagWInv.rows();

    // Create matB = R + lambda * Qt * W-1 * Q

    // Perform Cholesky decomposition of matB

    const auto& decomp = decomp_matB(matrices, lambda);


    const std::vector<t_matrix>& vec_gamma_L = do_vec_gamma(vec_Y, vecH, decomp);

    const std::vector<t_matrix>& g = do_vec_G(matrices, vec_gamma_L, vec_Y, lambda);

    // Calculate the influence matrix matA
    const DiagonalMatrixLD matA = diagonal_influence_matrix(matrices, 1, decomp, lambda);

    //long double long_GCV = 0.0L;
    const t_matrix long_trace = matA.diagonal().trace() / static_cast<t_matrix>(N); //compensated_sum(matA) / static_cast<long double>(N);
    const long double long_DLEc = 1.0L - long_trace;

    // Utilisation de l'algorithme de Kahan pour la sommation
    long double sum = 0.0L;
    long double c = 0.0L;  // Variable de compensation

    for (size_t i = 0; i < vec_Y.size(); ++i) {
        long double residual = g[i] - vec_Y[i];
        long double weight_inv = matrices.diagWInv.diagonal()[i];

        // Calculer (residual * residual / weight_inv) avec ordre optimal d'opérations
        long double term = (residual / std::sqrt(weight_inv)) * (residual / std::sqrt(weight_inv));

        // Sommation de Kahan
        long double y = term - c;
        long double t = sum + y;
        c = (t - sum) - y;
        sum = t;
    }


    sum /= static_cast<long double>(N);
    // GCV /= N/sum_wi; // ici ?? C'est une constante dans la recherche
    sum /= long_DLEc * long_DLEc;
    //

    return sum;
}

/**
 * @brief Calcule le critère REML (Restricted Maximum Likelihood) pour une spline cubique pénalisée.
 *
 * Formulation issue de :
 *   Wahba, G. (1985). "A comparison of GCV and REML for smoothing spline models."
 *   Annals of Statistics, 13(4), 1378–1402.
 *
 * Problème de lissage spline :
 * \f[
 *   \hat f = \arg\min_f \; \sum_{i=1}^N (y_i - f(x_i))^2 + \lambda \int (f''(t))^2 \, dt
 * \f]
 *
 * Critère REML à maximiser :
 * \f[
 *   \ell_\text{REML}(\lambda) = -\tfrac{1}{2} \Big[ (N - p)\,\log(\hat\sigma^2_\lambda)
 *   + \log |B_\lambda| + C \Big]
 * \f]
 *
 * où :
 * - \f$ \hat\sigma^2_\lambda = \frac{1}{N-p} (Y - \hat f_\lambda)^T W^{-1}(Y - \hat f_\lambda) \f$
 * - \f$ B_\lambda = R + \lambda Q^T W^{-1} Q \f$
 * - \f$ p \f$ = dimension de la base non pénalisée (typiquement 2 pour spline cubique)
 * - \f$ C \f$ = constante indépendante de \f$ \lambda \f$
 *
 * En pratique, on choisit \f$ \lambda \f$ qui minimise :
 * \f[
 *   -2 \ell_\text{REML}(\lambda) = (N - p)\,\log(\hat\sigma^2_\lambda) + \log |B_\lambda|
 * \f]
 *
 * @param vec_Y        Observations (vecteur Y)
 * @param matrices     Structures de matrices pré-calculées (R, Q, W^{-1}, etc.)
 * @param vecH         Paramètres réduits (ex. temps)
 * @param lambda       Coefficient de lissage
 * @param p            Dimension de la base polynomiale non pénalisée (typiquement 2)
 *
 * @return Valeur de \f$ -2 \ell_\text{REML}(\lambda) \f$ (à minimiser).
 */

long double restricted_likelihood (
    const std::vector<t_matrix> &vec_Y,
    const SplineMatrices& matrices,
    const std::vector<t_reduceTime> &vecH,
    const double lambda,
    const size_t p  // dimension de la partie polynomiale
    )
{
    const size_t N = matrices.diagWInv.rows();

    // 1. Décomposition de B_lambda
    const auto& decomp = decomp_matB(matrices, lambda);

    // 2. Estimation des coefficients et prédiction spline
    const std::vector<t_matrix>& vec_gamma_L = do_vec_gamma(vec_Y, vecH, decomp);
    const std::vector<t_matrix>& g = do_vec_G(matrices, vec_gamma_L, vec_Y, lambda);

    // 3. Calcul des résidus pondérés
    long double rss = 0.0L;  // residual sum of squares
    long double c = 0.0L;    // Kahan compensation
    for (size_t i = 0; i < vec_Y.size(); ++i) {
        long double residual = g[i] - vec_Y[i];
        long double w_inv = matrices.diagWInv.diagonal()[i];
        long double term = (residual * residual) / w_inv;

        long double y = term - c;
        long double t = rss + y;
        c = (t - rss) - y;
        rss = t;
    }

    // variance résiduelle estimée
    long double sigma2 = rss / static_cast<long double>(N - p);

    // 4. log |B_lambda| depuis la Cholesky de B
    const Matrix2D &L = decomp.first;
    long double logDetB = 0.0L;
    for (long long i = 0; i < L.rows(); ++i) {
        logDetB += 2.0L * std::log(L(i, i)); // produit des pivots
    }

    // 5. Calcul de -2*logL_REML
    long double reml = (N - p) * std::log(sigma2) + logDetB;

    return reml;  // plus petit = meilleur
}


double RSS(const std::vector<t_matrix> &vec_Y, const SplineMatrices &matrices, const std::vector<t_reduceTime> &vecH, const double lambda)
{
    Matrix2D matB (matrices.matR.rows(), matrices.matR.cols());
    if (lambda != 0) {
        const Matrix2D &tmp = multiConstParMat(matrices.matQTW_1Q, lambda, 5);
        matB = addMatEtMat(matrices.matR, tmp, 5);

    } else {
        matB = matrices.matR;
    }

    // Decomposition_Cholesky de matB en matL et matD
    // Si alpha global: calcul de Mat_B = R + alpha * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
    const std::pair<Matrix2D, DiagonalMatrixLD> &decomp = decompositionCholesky(matB, 5, 1);

    const SplineResults& sy = do_spline(vec_Y, matrices,  vecH, decomp, lambda);

    const std::vector< double>& vecG = sy.vecG;

    double res = 0.0;

    auto g = vecG.begin();
    for (auto& Y : vec_Y) {
        res += pow(Y - *g++, 2.0);
    }

    return std::move(res);

}

double var_Gasser(const std::vector<double>& vec_t, const std::vector<double>& vec_Y)
{
    double N = vec_t.size();
    double sum = 0.0;
    double c = 0.0; // Compensation

    for (auto i=1; i< N-1; i++) {
        double di = vec_t[i+1] - vec_t[i-1];
        double ai = di > 0 ?(vec_t[i+1] - vec_t[i]) / di : 0.5;
        double bi = 1.0 - ai;

        double ei = ai * vec_Y[i-1] + bi * vec_Y[i+1] - vec_Y[i];
        double ci2 = 1.0 / (1.0 + ai*ai + bi*bi);

        double term = ci2 * ei * ei;
        const double y = term - c;
        const double t = sum + y;
        c = (t - sum) - y;
        sum = t;

    }

    return sum / (N - 2.0);
}

t_matrix var_Gasser_2D(const std::vector<t_matrix>& vec_t, const std::vector<t_matrix>& vec_X, const std::vector<t_matrix>& vec_Y)
{
    t_matrix N = vec_t.size();
    t_matrix sum = 0.0;
    t_matrix c = 0.0; // Compensation

    for (auto i=1; i< N-1; i++) {
        // Pour chaque point i
        t_matrix di = vec_t[i+1] - vec_t[i-1];
        t_matrix ai = di > 0 ? (vec_t[i+1] - vec_t[i]) / di : 0.5;
        t_matrix bi = 1.0 - ai;

        // Calcul de l'erreur pour chaque dimension
        t_matrix ei_X = ai * vec_X[i-1] + bi * vec_X[i+1] - vec_X[i];
        t_matrix ei_Y = ai * vec_Y[i-1] + bi * vec_Y[i+1] - vec_Y[i];

        // Normalisation
        t_matrix ci2 = 1.0 / (1.0 + ai*ai + bi*bi);

        // Calcul de l'erreur quadratique (somme des carrés des erreurs dans les 2 dimensions)
        t_matrix term = ci2 * (ei_X * ei_X + ei_Y * ei_Y);

        // Sommation de Kahan pour une précision accrue
        const t_matrix y = term - c;
        const t_matrix t = sum + y;
        c = (t - sum) - y;
        sum = t;

    }

    return sum / (N - 2.0);
}


/**
 * @brief Computes a nonparametric local variance estimate for 3D trajectories
 *        using a Gasser–Müller-type estimator with numerical compensation (Kahan summation).
 *
 * This function evaluates how much each point deviates from a linear interpolation
 * between its neighbors, across the three spatial dimensions.
 *
 * Given time points \f$ t_0, \dots, t_{N-1} \f$ and associated 3D coordinates
 * \f$ \vec{Y}_i = (X_i, Y_i, Z_i) \in \mathbb{R}^3 \f$, the estimator computes:
 *
 * - Local interval:
 * \f[
 * d_i = t_{i+1} - t_{i-1}
 * \f]
 *
 * - Weights:
 * \f[
 * a_i = \frac{t_{i+1} - t_i}{d_i}, \quad b_i = 1 - a_i
 * \f]
 *
 * - Linear interpolation error in each dimension:
 * \f[
 * e_{i}^{(X)} = a_i X_{i-1} + b_i X_{i+1} - X_i \\
 * e_{i}^{(Y)} = a_i Y_{i-1} + b_i Y_{i+1} - Y_i \\
 * e_{i}^{(Z)} = a_i Z_{i-1} + b_i Z_{i+1} - Z_i
 * \f]
 *
 * - Normalization factor:
 * \f[
 * c_i^2 = \frac{1}{1 + a_i^2 + b_i^2}
 * \f]
 *
 * - Local variance contribution:
 * \f[
 * c_i^2 \cdot \left( (e_{i}^{(X)})^2 + (e_{i}^{(Y)})^2 + (e_{i}^{(Z)})^2 \right)
 * \f]
 *
 * Final result:
 * \f[
 * \text{Var}_\text{Gasser}^{(3D)} = \frac{1}{N - 2} \sum_{i=1}^{N-2} c_i^2 \cdot \left( (e_{i}^{(X)})^2 + (e_{i}^{(Y)})^2 + (e_{i}^{(Z)})^2 \right)
 * \f]
 *
 * @param vec_t A vector of time points (size N, strictly increasing).
 * @param vec_X A vector of X coordinates (same size as vec_t).
 * @param vec_Y A vector of Y coordinates (same size as vec_t).
 * @param vec_Z A vector of Z coordinates (same size as vec_t).
 *
 * @return The average locally-normalized squared deviation across the 3D trajectory.
 *
 * @note The computation excludes the first and last points (i = 1 to N-2).
 * @note Assumes all vectors are of equal size and N >= 3.
 */
t_matrix var_Gasser_3D(const std::vector<t_matrix>& vec_t, const std::vector<t_matrix>& vec_X, const std::vector<t_matrix>& vec_Y, const std::vector<t_matrix>& vec_Z )
{
    t_matrix N = vec_t.size();
    t_matrix sum = 0.0;
    t_matrix c = 0.0; // Compensation

    for (auto i = 1; i < N - 1; i++) {
        t_matrix di = vec_t[i + 1] - vec_t[i - 1];
        t_matrix ai = di > 0 ? (vec_t[i + 1] - vec_t[i]) / di : 0.5;
        t_matrix bi = 1.0 - ai;
        t_matrix ci2 = 1.0 / (1.0 + ai * ai + bi * bi);

        // Errors in each dimension
        t_matrix ei_X = ai * vec_X[i - 1] + bi * vec_X[i + 1] - vec_X[i];
        t_matrix ei_Y = ai * vec_Y[i - 1] + bi * vec_Y[i + 1] - vec_Y[i];
        t_matrix ei_Z = ai * vec_Z[i - 1] + bi * vec_Z[i + 1] - vec_Z[i];


        // Squared norm of the interpolation error
        t_matrix term = ci2 * (ei_X * ei_X + ei_Y * ei_Y + ei_Z * ei_Z);

        // Kahan summation
        const t_matrix y = term - c;
        const t_matrix t = sum + y;
        c = (t - sum) - y;
        sum = t;
    }

    return sum / (N - 2.0);
}


t_matrix var_Gasser(const std::vector<t_matrix>& vec_t, const std::vector<t_matrix>& vec_Y)
{
    const size_t N = vec_t.size();
    if (N <= 2) return 0.0L; // Protection contre les cas limites

    // Utilisation de la sommation de Kahan pour améliorer la précision
    t_matrix sum = 0.0L;
    t_matrix c = 0.0L; // Variable de compensation

#pragma omp parallel for reduction(+:sum) // Parallélisation si OpenMP est disponible
    for (size_t i = 1; i < N - 1; ++i) {
        const t_matrix di = vec_t[i+1] - vec_t[i-1];
        const t_matrix ai = di > 0.0L ? (vec_t[i+1] - vec_t[i]) / di : 0.5L;
        const t_matrix bi = 1.0L - ai;

        const t_matrix ei = ai * vec_Y[i-1] + bi * vec_Y[i+1] - vec_Y[i];

        const t_matrix ci2 = 1.0L / (1.0L + ai*ai + bi*bi);
        const t_matrix term = ci2 * ei * ei;

// Sommation de Kahan (si non parallélisé)
#ifndef _OPENMP
        const t_matrix y = term - c;
        const t_matrix t = sum + y;
        c = (t - sum) - y;
        sum = t;
#else
        sum += term;
#endif
    }

    return sum / static_cast<long double>(N - 2);
}

// Gasser avec utilisation de la matrice Y_mat des composants
t_matrix var_Gasser(const std::vector<t_matrix>& vec_t, const Matrix2D& Y_mat)
{
    const size_t n_points = vec_t.size();

#ifdef DEBUG
    if (n_points < 3) {
        return 0.0; // pas assez de points pour estimer
    }
#endif

    const long long n_composents = Y_mat.cols();
    t_matrix sum = 0.0;
    t_matrix c = 0.0; // Compensation pour Kahan

    for (size_t i = 1; i < n_points - 1; i++) {
        const t_matrix di = vec_t[i + 1] - vec_t[i - 1];
        const t_matrix ai = di > 0 ? (vec_t[i + 1] - vec_t[i]) / di : 0.5;
        const t_matrix bi = 1.0 - ai;

        t_matrix norm2 = 0.0;

        // Calcul de l’erreur par dimension
        for (long long d = 0; d < n_composents; ++d) {
            const t_matrix ei = ai * Y_mat(i - 1, d)
            + bi * Y_mat(i + 1, d)
                -      Y_mat(i, d);
            norm2 += ei * ei;
        }

        const t_matrix ci2 = 1.0 / (1.0 + ai * ai + bi * bi);
        const t_matrix term = ci2 * norm2;

        // Somme de Kahan
        const t_matrix y = term - c;
        const t_matrix t = sum + y;
        c = (t - sum) - y;
        sum = t;
    }

    return sum / (n_points - 2.0);
}


// Cross-validation initialization
std::pair<double, double> initLambdaSplineByCV(const bool depth, const std::vector<t_matrix> &vec_X, const std::vector<t_matrix> &vec_X_err, const SplineMatrices &matrices, const std::vector<t_reduceTime> &vecH,const std::vector<t_matrix> &vec_Y, const std::vector<t_matrix> &vec_Z)
{
    std::vector<double> GCV, CV, lambda_GCV, lambda_CV, lambda_GCV_Vg, lambda_CV_Vg;

    double Vg;
    bool doY = !vec_Y.empty() && vec_Z.empty();
    bool doYZ = !vec_Y.empty() && !vec_Z.empty();

    for (int idxLambda = -200; idxLambda < 101; ++idxLambda ) {
        const double lambda_loop = pow(10., ( double)idxLambda/10.);

        Vg = var_residual(vec_X, matrices, vecH, lambda_loop);

        if (doY) {
            Vg += var_residual(vec_Y, matrices, vecH, lambda_loop);
            Vg /= 2.;

        } else if (doYZ) {
            Vg += var_residual(vec_Y, matrices, vecH, lambda_loop) + var_residual(vec_Z, matrices, vecH, lambda_loop);
            Vg /= 3.;
        }


        DiagonalMatrixLD vec_Vg_Si2 (vec_X_err.size());
        int i = 0;
        for (auto X_err : vec_X_err) {
            vec_Vg_Si2.diagonal()[i++] = Vg + X_err * X_err;
        }

        const SplineMatrices &test_matrices = prepare_calcul_spline(vecH, vec_Vg_Si2);

        //-----
        const double cv = cross_validation(vec_X, test_matrices, vecH, lambda_loop);
        const double gcv = general_cross_validation(vec_X, test_matrices, vecH, lambda_loop);


        if (!std::isnan(gcv)) { // && !std::isinf(gcv)) {
            GCV.push_back(gcv);
            lambda_GCV.push_back(lambda_loop);
            lambda_GCV_Vg.push_back(Vg);
            //qDebug()<<" gcv="<<idxLambda<<GCV.back();//<< has_positif;
        }

        if (!std::isnan(cv)) {// && !std::isinf(cv)) {
            CV.push_back(cv);
            lambda_CV.push_back(lambda_loop);
            lambda_CV_Vg.push_back(Vg);
            //qDebug()<<" cv="<<idxLambda<<CV.back();//<< has_positif;
        }
    }

    // We are looking for the smallest CV value
    unsigned long idxDifMin = std::distance(GCV.begin(), std::min_element(GCV.begin(), GCV.end()) );

    unsigned long idxDifMinCV = std::distance(CV.begin(), std::min_element(CV.begin(), CV.end()) );


    // If the mini is at one of the bounds, there is no solution in the interval for GCV
    // See if there is a solution in CV


    std::vector<double>* lambda_vect;
    std::vector<double>* Vg_vect;
    int idx_vect;

     if (idxDifMin == 0 || idxDifMin == (GCV.size()-1)) {
        qDebug()<<" 2d chance\t idxDifMin CV ="<<idxDifMinCV<< "Pas de solution avec GCV idxDifMin GCV = 00 "<<idxDifMin;

        if (idxDifMinCV == 0 || idxDifMinCV == (CV.size()-1)) {
            qDebug()<<"[initLambdaSplineByCV] No solution with CV ="<<idxDifMinCV<< " On prend lambda_GCV[0] = 10E"<<log10(lambda_GCV.at(0));

            idx_vect = 0;
            lambda_vect = &lambda_GCV;
            Vg_vect = &lambda_GCV_Vg;

        } else {
            qDebug()<<"[initLambdaSplineByCV] With CV lambda_CV.at("<<idxDifMinCV<<")= 10E"<<log10(lambda_CV.at(idxDifMinCV));

            idx_vect = idxDifMinCV;
            lambda_vect = &lambda_CV;
            Vg_vect = &lambda_CV_Vg;
        }

     } else {
        qDebug()<<"[initLambdaSplineByCV] With GCV lambda_GCV.at("<<idxDifMin<<")= 10E"<<log10(lambda_GCV.at(idxDifMin));

        idx_vect = idxDifMin;
        lambda_vect = &lambda_GCV;
        Vg_vect = &lambda_GCV_Vg;
     }


     // test positif

     if (depth) {

        bool has_positif = false;

        int idx_test = idx_vect;
        double lambda_test, Vg_test;
        do {
            lambda_test =  lambda_vect->at(idx_test);
            Vg_test = Vg_vect->at(idx_test);

            /*std::vector<long double> vec_Vg_Si2;
            for (auto X_err : vec_X_err) {
                vec_Vg_Si2.push_back(Vg_test + pow(X_err, 2.));
            }*/
            DiagonalMatrixLD vec_Vg_Si2 (vec_X_err.size());
            int i =0;
            for (auto X_err : vec_X_err) {
                vec_Vg_Si2.diagonal()[i++] = (Vg_test + X_err * X_err);
            }
            const SplineMatrices &spline_matrices = prepare_calcul_spline(vecH, vec_Vg_Si2);

            //  Spline final

            const Matrix2D &tmp = multiConstParMat(spline_matrices.matQTW_1Q, lambda_test, 5);
            const Matrix2D &matB = addMatEtMat(spline_matrices.matR, tmp, 5);


            // Decomposition_Cholesky de matB en matL et matD
            // Si alpha global: calcul de Mat_B = R + alpha * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
            const std::pair<Matrix2D, DiagonalMatrixLD> &decomp = decompositionCholesky(matB, 5, 1);

            const SplineResults &sx = do_spline(vec_X, spline_matrices,  vecH, decomp, lambda_test);

            const std::vector<double> &vecVarG = calcul_spline_variance(spline_matrices, decomp, lambda_test);

            // recomposition de vec_theta_red
            std::vector<double> vec_theta_red;
            double som = 0;
            vec_theta_red.push_back(som);
            for (auto& h : vecH) {
                som += h;
                vec_theta_red.push_back(som);
            }


            MCMCSplineComposante splineX;
            splineX.vecThetaReduced = vec_theta_red;
            splineX.vecG = std::move(sx.vecG);
            splineX.vecGamma = std::move(sx.vecGamma);

            splineX.vecVarG = vecVarG;

            /*for (int i = 0; i < events.size(); i++) {
            events[i]->mGx = splineX.vecG[i];
            }*/

            MCMCSpline spline;
            spline.splineX = std::move(splineX);

            has_positif = hasPositiveGPrimeByDet(spline.splineX);
            if (!has_positif) {
                idx_test++;
                if (idx_test > (int)Vg_vect->size()-1) {
                   has_positif = true;
                }
            }

        } while (!has_positif && lambda_test<1E10);

        //------
        return std::make_pair(lambda_test, Vg_test);

    } else {

        return std::make_pair(lambda_vect->at(idx_vect), Vg_vect->at(idx_vect));
    }

    // ----- RETURN
    return std::make_pair(1., 0.);

}
#pragma mark Silvermann

std::pair<double, double> initLambdaSplineBySilverman(SilvermanParam& sv, const std::vector<t_matrix>& vec_X, const std::vector<t_matrix>& vec_X_err, const std::vector<t_reduceTime> &vecH, const std::vector<t_matrix> &vec_Y, const std::vector<t_matrix> &vec_Y_err, const std::vector<t_matrix> &vec_Z, const std::vector<t_matrix> &vec_Z_err)
{
    std::vector<long double> GCV, CV;
    std::vector<double> lambda_GCV, lambda_CV;
    const bool doY = !vec_Y.empty() && vec_Z.empty();
    const bool doYZ = !vec_Y.empty() && !vec_Z.empty();

    long long i = 0;
    DiagonalMatrixLD W_1 (vec_X_err.size());
    if (sv.use_error_measure) {

        if (doY) {
            auto Y_err = vec_Y_err.begin();
            for (auto &X_err : vec_X_err) {
                const double Sy = pow(X_err, -2.0) + pow(*Y_err++, -2.0);
                W_1.diagonal()[i++] = 2.0/ Sy;
            }
        }
        else if (doYZ) {
            auto Y_err = vec_Y_err.begin();
            auto Z_err = vec_Z_err.begin();
            for (auto &X_err : vec_X_err) {
                const double Sy = pow(X_err, -2.) + pow(*Y_err++, -2.0) + pow(*Z_err++, -2.0);
                W_1.diagonal()[i++] = 3.0/ Sy;
            }
        }
        else {
            for (auto &X_err : vec_X_err) {
                W_1.diagonal()[i++] = X_err * X_err;
            }
        }

    } else {
        W_1.setIdentity();
    }

    // old code
    /*
    t_matrix ONE_sum_W = 1.0 / W_1.diagonal().sum();
    DiagonalMatrixLD W_1_normalized = W_1 * ONE_sum_W;
    const SplineMatrices test_matrices = prepare_calcul_spline(vecH, W_1);
    */

    // Pour calcul matriciel
    const Matrix2D R = calculMatR(vecH);

    const Matrix2D Q = calculMatQ(vecH);

    const Matrix2D Qt =  Q.transpose();
    size_t n = vec_X.size();

    DiagonalMatrixLD I (n);
    I.setIdentity();


    Matrix2D Y_mat(n, 1);
    // Construction du vecteur x
    ColumnVectorLD x_vec, y_vec, z_vec;

    x_vec.resize(n);
    for (size_t i = 0; i < n; ++i)
        x_vec(i) = vec_X[i];

    Y_mat << x_vec;

    if (doY) {
        Y_mat.resize(n, 2);
        y_vec.resize(n);
        for (size_t i = 0; i < n; ++i)
            y_vec(i) = vec_Y[i];

        Y_mat << x_vec, y_vec;

    } else if (doYZ) {
        Y_mat.resize(n, 3);
        y_vec.resize(n);
        z_vec.resize(n);
        for (size_t i = 0; i < n; ++i) {
            y_vec(i) = vec_Y[i];
            z_vec(i) = vec_Z[i];
        }

        Y_mat << x_vec, y_vec, z_vec;
    }

    size_t n_points = Y_mat.rows();
    size_t n_components = Y_mat.cols();

    //
    std::vector<double> lambdas;        // les λ testés
    std::vector<long double> GCV_vals;  // valeurs brutes GCV
    std::vector<long double> CV_vals;   // valeurs brutes CV
    //std::vector<long double> REML_vals;   // valeurs brutes REML

    double best_lambda_cv = 0.0, best_lambda_gcv = 0.0;//, best_lambda_reml = 0.0; //best_lambda_rss = 0.0

    double best_cv = std::numeric_limits<double>::infinity();
    double best_gcv = std::numeric_limits<double>::infinity();
    //double best_reml = std::numeric_limits<double>::infinity();

    for (int lambda_loop_exp = -1000; lambda_loop_exp < 1001; lambda_loop_exp+= 5) {//++lambda_loop_exp ) {

        const double lambda = pow(10.0, static_cast<double>(lambda_loop_exp)/100.0);

        // Opération matricielle

        Matrix2D B = R + lambda * Qt * W_1 * Q;

        Eigen::LDLT<Matrix2D> solver(B);
        auto B_1Qt = solver.solve(Qt); //  -> x = B_1 * y -> B_1Qt = B_1 * Qt
        auto QB_1Qt = Q * B_1Qt;

        // Matrice de projection (ou opérateur de lissage).
        // Il faut définir le type de A, sinon il bug lors du calcul de la trace !!
        Matrix2D A = I.toDenseMatrix() - lambda * W_1 * QB_1Qt;// forme classique

        Matrix2D Y_hat = A * Y_mat ;
        double traceA = A.trace();

        // =====================
        // 2️⃣ GCV pondéré
        // =====================
        double weighted_rss = 0.0;
        for (size_t i = 0; i < n_points; ++i) {
            double w_ii = 1.0 / W_1.diagonal()[i]; // récupérer W_{ii} (car W_1 = W^{-1})
            for (size_t j = 0; j < n_components; ++j) {
                double res = Y_mat(i, j) - Y_hat(i, j);
                weighted_rss += (res * res) / w_ii;
            }
        }
        double dle   = 1.0 - (traceA / n_points); // degrees of freedom correction
        double gcv_m   = (weighted_rss / (n_points * n_components)) / (dle * dle);


        // =====================
        // 3️⃣ CV pondéré (LOOCV)
        // =====================
        double cv_m = 0.0;
        for (size_t i = 0; i < n_points; ++i) {
            double denom = 1.0 - A(i, i);
            double w_ii  = 1.0 / W_1.diagonal()[i]; // W_{ii}
            for (size_t j = 0; j < n_components; ++j) {
                double err = (Y_mat(i, j) - Y_hat(i, j)) / denom;
                cv_m += (err * err) / w_ii;
            }
        }
        cv_m /= (n_points * n_components);


        // =====================
        // 4️⃣ REML
        // =====================
        /**
         * Calcule le critère REML (Restricted Maximum Likelihood) pour une spline cubique pénalisée.
         *
         * Formulation issue de :
         *   Wahba, G. (1985). "A comparison of GCV and REML for smoothing spline models."
         *   Annals of Statistics, 13(4), 1378–1402.
         *
         * Problème de lissage spline :
         * \f[
         *   \hat f = \arg\min_f \; \sum_{i=1}^N (y_i - f(x_i))^2 + \lambda \int (f''(t))^2 \, dt
         * \f]
         *
         * Critère REML à maximiser :
         * \f[
         *   \ell_\text{REML}(\lambda) = -\tfrac{1}{2} \Big[ (N - p)\,\log(\hat\sigma^2_\lambda)
         *   + \log |B_\lambda| + C \Big]
         * \f]
         *
         * où :
         * - \f$ \hat\sigma^2_\lambda = \frac{1}{N-p} (Y - \hat f_\lambda)^T W^{-1}(Y - \hat f_\lambda) \f$
         * - \f$ B_\lambda = R + \lambda Q^T W^{-1} Q \f$
         * - \f$ p \f$ = dimension de la base non pénalisée (typiquement 2 pour spline cubique)
         * - \f$ C \f$ = constante indépendante de \f$ \lambda \f$
         *
         * En pratique, on choisit \f$ \lambda \f$ qui minimise :
         * \f[
         *   -2 \ell_\text{REML}(\lambda) = (N - p)\,\log(\hat\sigma^2_\lambda) + \log |B_\lambda|
         * \f]
         */

        // 1. Nombre d'observations effectives
        /*size_t N = n_points * n_components; // toutes les observations
        size_t p = 2; // spline cubique pénalisée : base polynomiale de degré <=1 -> dimension = 2

        // 2. Variance résiduelle estimée
        double sigma2_hat = weighted_rss / static_cast<double>(N - p);

        // 3. log |B_lambda| à partir de la décomposition LDLT
        double logDetB = 0.0;
        auto D = solver.vectorD();   // valeurs diagonales de D dans LDLT
        for (int i = 0; i < D.size(); ++i) {
            logDetB += std::log(std::abs(D(i)));
        }

        // 4. Critère REML
        long double reml = (N - p) * std::log(sigma2_hat) + logDetB;
        */
        // (on néglige les constantes indépendantes de lambda)

        // → Plus petit reml = meilleur choix de lambda

        // fin op mat



        // old code
        /*


        long double cv = cross_validation(vec_X, test_matrices, vecH, lambda);

        if (doY) {
            cv += cross_validation(vec_Y, test_matrices, vecH, lambda);
            cv /= 2.0;

        }  else if (doYZ) {
            cv += cross_validation(vec_Y, test_matrices, vecH, lambda);
            cv += cross_validation(vec_Z, test_matrices, vecH, lambda);
            cv /= 3.0;
        }

        long double gcv = general_cross_validation(vec_X, test_matrices, vecH, lambda);


        if (doY) {
            gcv += general_cross_validation(vec_Y, test_matrices, vecH, lambda);
            gcv /= 2.0;

        }  else if (doYZ) {
            gcv += general_cross_validation(vec_Y, test_matrices, vecH, lambda);
            gcv += general_cross_validation(vec_Z, test_matrices, vecH, lambda);
            gcv /= 3.0;
        }
        */
        //const double rss = RSS(vec_X, test_matrices, vecH, lambda_loop);


        if (gcv_m < best_gcv) {
            best_gcv = gcv_m;
            best_lambda_gcv = lambda;
        }

        if (cv_m < best_cv) {
            best_cv = cv_m;
            best_lambda_cv = lambda;
        }

        //long double reml = restricted_likelihood(vec_X, test_matrices, vecH, lambda);

        // Sauvegarde dans vecteurs + tables
        lambdas.push_back(lambda);
        GCV_vals.push_back(gcv_m);
        CV_vals.push_back(cv_m);
       // REML_vals.push_back(reml);

        sv.tab_GCV[lambda] = gcv_m;
        sv.tab_CV[lambda]  = cv_m;

    }


    // --------------------------------------------------
    // Filtrage + recherche du min pour GCV et CV
    // --------------------------------------------------
    auto process_min = [&](std::vector<long double> vals) {
        // Supprimer zéros initiaux
        auto it = std::find_if(vals.begin(), vals.end(),
                               [](long double v) { return v != 0.0; });
        size_t nb_0 = std::distance(vals.begin(), it);
        if (it != vals.begin())
            vals.erase(vals.begin(), it);

        // Filtre gaussien
        constexpr int padding_type = 1;
        double sigma_filter= vals.size() / 20.0;
        vals = gaussian_filter(vals, sigma_filter, padding_type);

        // Recherche min
        size_t idx_min = std::distance(vals.begin(),
                                       std::min_element(vals.begin(), vals.end()));
        idx_min += nb_0; // correction index après suppression zéros

        return idx_min;
    };

    size_t idx_gcv = process_min(GCV_vals);
    size_t idx_cv  = process_min(CV_vals);
    //size_t idx_reml  = process_min(REML_vals);
    best_lambda_gcv = lambdas[idx_gcv];
    best_lambda_cv  = lambdas[idx_cv];
    //best_lambda_reml  = lambdas[idx_reml];

    std::cout << "[initLambdaSplineBySilverman]"
              << "Lambda optimal (GCV) = " << best_lambda_gcv
              << " GCV min = " << GCV_vals[idx_gcv] << std::endl;

    std::cout << "[initLambdaSplineBySilverman]"
              << "Lambda optimal (CV)  = " << best_lambda_cv
              << " CV min  = " << CV_vals[idx_cv] << std::endl;

   /* std::cout << "[initLambdaSplineBySilverman]"
              << "Lambda optimal (REML) = " << best_lambda_reml
              << " REML min  = " << REML_vals[idx_reml] << std::endl;
   */
    double lambda_mini;

    if (idx_gcv > 0 && idx_gcv < static_cast<size_t>(lambdas.size() - 1)) {
        // Solution évidente avec GCV
        lambda_mini = best_lambda_gcv;
        sv.comment = "GCV solution; ";

    } else if (idx_cv > 0 && idx_cv < static_cast<size_t>(lambdas.size() - 1)) {
        // Solution évidente avec CV
        lambda_mini = best_lambda_cv;
        sv.comment = "CV solution; ";

    } /* else if (idx_reml > 0 && idx_reml < static_cast<size_t>(lambdas.size() - 1)) {
        // Solution évidente avec REML
        lambda_mini = best_lambda_reml;
        sv.comment = "REML solution; ";

    }*/ else {
        // Pas de minimum clair
        // Si CV diminue continûment vers λ→0, cela suggère qu’un lissage quasi nul (donc spline presque interpolante) est préféré.
        // Si au contraire CV/GCV diminue quand  λ→∞, alors un lissage extrême (droite ou polynôme bas degré) est préféré.
        if (GCV_vals.front() > GCV_vals.back()) {
            lambda_mini = 1.0e20;  // Lin. regression
            sv.comment = "No GCV solution; Linear Regression; ";
        } else {
            lambda_mini = 0.0;     // Interpolation spline
            sv.comment = "No GCV solution; Spline Interpolation; ";
        }
    }

    // Opération matricielle avec lambda_mini
    //lambda_mini = 1.0e-6; // test
    Matrix2D B = R + lambda_mini * Qt * W_1 * Q;

    // Identique à solver(B)
    /*auto Bm1 = inverse_padded_matrix(B);
    auto QBm1Qt = Q * Bm1* Qt;
    showMatrix(QBm1Qt, "QBm1Qt");*/

    Eigen::LDLT<Matrix2D> solver(B);
    auto B_1Qt = solver.solve(Qt); //  -> x = B_1 * y ; on remplace y par Qt donc B_1Qt=x= B_1 * Qt
    auto QB_1Qt = Q * B_1Qt;

    // matrice de projection (ou opérateur de lissage).
    // Il faut définir le type de A à Matrix2D, sinon il a un bug lors du calcul de la trace !!
    Matrix2D A = I.toDenseMatrix() - static_cast<t_matrix>(lambda_mini) * W_1 * QB_1Qt;// forme classique

    Matrix2D Y_hat = A * Y_mat ;
    // EDF (effective degrees of freedom)
    //auto EDF_lissage = A.trace();

    auto EDF_residuel = n_points - A.trace();

    // Calcul de la variance résiduelle
    double Vg = 0.0;
    /*for (int j = 0; j < Y_mat.cols(); ++j) {
        Eigen::Vector<t_matrix, Dynamic> residual = Y_mat.col(j) - Y_hat.col(j);
        Vg += residual.squaredNorm() / EDF_residuel;
    }
    */
    // Même calcul, mais plus explicite
    for (int j = 0; j < Y_mat.cols(); ++j) {
        for (int k = 0; k < Y_mat.rows(); ++k) {
            auto residual = Y_mat(k, j) - Y_hat(k, j);
            Vg += residual * residual / EDF_residuel;
        }
    }
    Vg /= Y_mat.cols(); // moyenne sur les composantes


    //  _________________ old code

    /*
        double Vg = var_residual(vec_X, test_matrices, vecH, lambda_mini);

        if (doY) {
            Vg += var_residual(vec_Y, test_matrices, vecH, lambda_mini);
            Vg /= 2.;

        } else if (doYZ) {
            Vg += var_residual(vec_Y, test_matrices, vecH, lambda_mini) + var_residual(vec_Z, test_matrices, vecH, lambda_mini);
            Vg /= 3.;
        }
    */

    return std::make_pair(lambda_mini, Vg);

}

/**
 * @brief var_residual , compute the Residual Sum of Squares about the fitted curve.
 * see Green and Silverman
 * @param vec_Y
 * @param matrices
 * @param vecH
 * @param lambda
 * @return
 */
double var_residual(const std::vector<t_matrix>& vec_Y, const SplineMatrices& matrices, const std::vector<t_reduceTime>& vecH, const double lambda)
{
    std::unique_ptr<std::pair<Matrix2D, DiagonalMatrixLD>> decomp;

    // Decomposition_Cholesky de matB en matL et matD
    if (lambda != 0.0) {
        decomp = std::make_unique<std::pair<Matrix2D, DiagonalMatrixLD>>(decomp_matB(matrices, lambda));

    } else {
        decomp = std::make_unique<std::pair<Matrix2D, DiagonalMatrixLD>>(decompositionCholesky(matrices.matR, 5, 1));
    }

    //  Calcul de Mat_B = R + lambda * Qt * W-1 * Q
    /*auto L = decomp->first;
    auto D = decomp->second;
    auto verif_R = L * D * L.transpose();
    showMatrix(verif_R, "verif_R");*/

    const SplineResults& sy = do_spline(vec_Y, matrices,  vecH, *decomp, lambda);
    //Calcul seulement la diagonal de A
    const DiagonalMatrixLD diag_A = diagonal_influence_matrix(matrices, 1, *decomp, lambda);

    long double N = static_cast<long double>(vec_Y.size());

    long double res = 0.0L;
    const long double trace = diag_A.toDenseMatrix().trace();// compensated_sum(matA);

    for (size_t i = 0; i < vec_Y.size(); ++i) {
        long double residual = static_cast<long double>(sy.vecG[i] - vec_Y[i]);
        res += residual * residual  ;
    }
    long double EDF = N - trace;

    res /= EDF;

    return std::move(res);

}

std::vector<double> general_residual(const std::vector<t_matrix> &vec_Y,  const SplineMatrices &matrices, const std::vector<t_reduceTime> &vecH, const double lambda)
{
    const double N = matrices.diagWInv.rows();

    const Matrix2D& matB = addMatEtMat(matrices.matR, multiConstParMat(matrices.matQTW_1Q, lambda, 5), 5);

    // Decomposition_Cholesky de matB en matL et matD
    // Si alpha global: calcul de Mat_B = R + lambda * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
    //std::pair<Matrix2D, std::vector<double>> decomp = decompositionCholesky(matB, 5, 1);
    //SplineResults s = calculSplineX (matrices, vecH, decomp, matB, lambdaSpline);

    //auto matA = diagonal_influence_matrix(matrices, s , 1, lambdaSpline);
    const auto& decomp = decompositionCholesky(matB, 5, 1);

    const SplineResults &s = do_spline(vec_Y, matrices,  vecH, decomp, lambda);
    const DiagonalMatrixLD matA = diagonal_influence_matrix(matrices, 1, decomp, lambda);

    // Nombre de degré de liberté
    //const auto DLEc = 1 - std::accumulate(matA.begin(), matA.end(), 0.0) / static_cast<t_matrix>(N);
    const auto DLEc = 1 - matA.diagonal().trace() / static_cast<t_matrix>(N);
    // 1 - calcul de sigma2 star
    /*double sum_w = 0.;
    for (auto& w_1 : matrices.diagWInv) {
        sum_w += 1./w_1;
    }
    sum_w /= (double)matrices.diagWInv.size() ;
    */
   /* double sig2 = var_residual(vec_Y, matrices, vecH, lambda)*sum_w;
    for (int i = 0 ; i < N; i++) {
        sig2 +=  pow( vec_Y[i] - s.vecG.at(i), 2) /matrices.diagWInv.at(i);
    }
   */

    // 2- calcul des ri star

    double sigma = 1.0;//sqrt(var);
    std::vector<double> g_res;
    for (int i = 0 ; i < N; i++) {
        g_res.push_back(  ( vec_Y[i] - s.vecG.at(i))/ (sigma*sqrt(matrices.diagWInv.diagonal()[i])*sqrt(DLEc)) );
    }

    return g_res;
}

bool  hasPositiveGPrimeByDet (const MCMCSplineComposante &splineComposante)
{

    for (unsigned long i= 0; i< splineComposante.vecThetaReduced.size()-1; i++) {

        const double t_i = splineComposante.vecThetaReduced.at(i);
        const double t_i1 = splineComposante.vecThetaReduced.at(i+1);
        const double hi = t_i1 - t_i;

        const double gamma_i = splineComposante.vecGamma.at(i);
        const double gamma_i1 = splineComposante.vecGamma.at(i+1);

        const double g_i = splineComposante.vecG.at(i);
        const double g_i1 = splineComposante.vecG.at(i+1);

        const double a = (g_i1 - g_i) /hi;
        const double b = (gamma_i1 - gamma_i) /(6*hi);
        const double s = t_i + t_i1;
        const double p = t_i * t_i1;
        const double d = ( (t_i1 - 2*t_i)*gamma_i1 + (2*t_i1 - t_i)*gamma_i )/(6*hi);
        // résolution équation

        const double aDelta = 3* b;
        const double bDelta = 2*d - 2*s*b;
        const double cDelta = p*b - s*d + a;

        const double delta = pow(bDelta, 2.) - 4*aDelta*cDelta;
        if (delta < 0) {
            if (aDelta < 0) // convexe
                return false;
            else           // concave
                continue;
        }

        double t1_res = (-bDelta - sqrt(delta)) / (2*aDelta);
        double t2_res = (-bDelta + sqrt(delta)) / (2*aDelta);

        if (t1_res > t2_res)
            std::swap(t1_res, t2_res);


        if (aDelta > 0) { //C'est un maximum entre les solutions
            if (!( t_i1 < t1_res || t2_res< t_i)) {
                return false;
            }

        } else { //C'est un minimum entre les solutions
            if ( !( t1_res < t_i && t_i1 < t2_res) )
                return false;
        }


        /*
        if (a < 0) {
            return false;

        } else if ( t_i < t1_res && t1_res< t_i1) {
            return false;

        } else if ( t_i < t2_res && t2_res< t_i1) {
            return false;
        }
*/
    }

    return true;
}

void spread_theta_reduced(std::vector<t_reduceTime> &sorted_t_red, t_reduceTime spread_span)
{
    std::vector<t_reduceTime>::iterator it_first = sorted_t_red.end();
    std::vector<t_reduceTime>::iterator it_last = sorted_t_red.end();
    unsigned nbEgal = 0;

    if (spread_span == 0.) {
        spread_span = 1.E-8; //std::numeric_limits<double>::epsilon() * 1.E12;//1.E6;// epsilon = 1E-16
    }

    // repère première egalité
    for (std::vector<t_reduceTime>::iterator it = sorted_t_red.begin(); it != sorted_t_red.end() -1; it++) {

        if (*std::next(it) - *it <= spread_span) {

            if (it_first == sorted_t_red.end()) {
                it_first = it;
                it_last = it + 1;
                nbEgal = 2;

            } else {
                it_last = it + 1;
                ++nbEgal;
            }

        } else {
            if (it_first != sorted_t_red.end()) {
                // on sort d'une égalité, il faut répartir les dates entre les bornes
                // itEvent == itEventLast
                const t_reduceTime lowBound = it_first == sorted_t_red.begin() ? sorted_t_red.front() : *std::prev(it_first) ; //valeur à gauche non égale
                const t_reduceTime upBound = it == sorted_t_red.end()-2 ? sorted_t_red.back(): *std::next(it);

                t_reduceTime step = spread_span / (nbEgal-1); // écart théorique
                t_reduceTime min;

                // Controle du debordement sur les valeurs encadrantes
                if (it_first == sorted_t_red.begin()) {
                   // Cas de l'égalité avec la première valeur de la liste
                   // Donc tous les Events sont à droite de la première valeur de la liste
                   min = *it;

                } else {
                   // On essaie de placer une moitier des Events à gauche et l'autre moitier à droite
                   min = *it- step*floor(nbEgal/2.);
                   // controle du debordement sur les valeurs encadrantes
                   min = std::max(lowBound + step, min );
                }

                const double max = std::min(upBound - spread_span, *it + (double)(step*ceil(nbEgal/2.)) );
                step = (max- min)/ (nbEgal - 1); // écart corrigé

                std::vector<t_reduceTime>::iterator it_egal;
                int count;
                for (it_egal = it_first, count = 0; it_egal != it+1; it_egal++, count++ ) {
                   *it_egal = min + count*step;
                }
                // Fin correction, prêt pour nouveau groupe/cravate
                it_first = sorted_t_red.end();

            }
        }


    }

    // sortie de la boucle avec itFirst validé donc itEventLast == sortedEvents.end()-1

    if (it_first != sorted_t_red.end()) {
        // On sort de la boucle et d'une égalité, il faut répartir les dates entre les bornes
        // itEvent == itEventLast
        const t_matrix lowBound = *std::prev(it_first); //la première valeur à gauche non égale

        const t_matrix max = sorted_t_red.back();
        t_matrix step = spread_span / (nbEgal-1.); // ecart théorique

        const t_matrix min = std::max(lowBound + spread_span, max - step*(nbEgal-1) );

        step = (max- min)/ (nbEgal-1); // écart corrigé

        // Tout est réparti à gauche
        int count;
        std::vector<t_reduceTime>::iterator it_egal;
        for (it_egal = it_first, count = 0; it_egal != sorted_t_red.end(); it_egal++, count++ ) {
            *it_egal = min + count *step;
        }

    }

}

/**
 * @brief get_order returns the indices that order the data in the vector
 * @param vec
 * @return
 */
std::vector<int> get_order(const std::vector<t_reduceTime> &vec)
{
    struct vec_index {
        vec_index(double _t, int _idx) {
            t = _t;
            idx = _idx;
        }
        double t;
        int idx;
    };
    std::vector<vec_index> tmp;
    int i = 0;
    for (auto t : vec)
        tmp.push_back(vec_index(t, i++));

    std::sort(tmp.begin(), tmp.end(), [](const vec_index a, const vec_index b) { return (a.t < b.t); });

    std::vector<int> res;
    for (auto t_i : tmp)
        res.push_back(t_i.idx);

    return res;
}

std::pair<Matrix2D, DiagonalMatrixLD> decomp_matB(const SplineMatrices& matrices, const double lambdaSpline)
{
    Q_ASSERT(lambdaSpline != 0);
    /*
    * if lambdaSpline == 0, it is interpolation
    * return decompositionCholesky(matrices.matR, 5, 1);
    */

    // Decomposition_Cholesky de matB en matL et matD
    // Si lambda global: calcul de Mat_B = R + lambda * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D
    const Matrix2D &tmp = multiConstParMat(matrices.matQTW_1Q, lambdaSpline, 5);

    const Matrix2D &matB = addMatEtMat(matrices.matR, tmp, 5);
    return decompositionCholesky(matB, 5, 1);

}

#pragma mark usefull math function for MCMCLoopCurve

/**
 * Calcul de h_YWI_AY pour toutes les composantes de Y event (suivant la configuration univarié, spérique ou vectoriel)
 */
t_prob h_YWI_AY(const SplineMatrices& matrices, const std::vector<std::shared_ptr<Event>> &events, const double lambdaSpline, const std::vector< double>& vecH, const bool hasY, const bool hasZ)
{
    (void) hasZ;
    const Matrix2D& matR = matrices.matR;

    Matrix2D matB;

    if (lambdaSpline != 0) {
        // Decomposition_Cholesky de matB en matL et matD
        // Si lambda global: calcul de Mat_B = R + lambda * Qt * W-1 * Q  et décomposition de Cholesky en Mat_L et Mat_D

        const Matrix2D &tmp = multiConstParMat(matrices.matQTW_1Q, lambdaSpline, 5);
        matB = addMatEtMat(matR, tmp, 5);

    } else {
        matB = matR;
    }

    const std::pair<Matrix2D, DiagonalMatrixLD> &decomp_matB = decompositionCholesky(matB, 5, 1);
    const std::pair<Matrix2D, DiagonalMatrixLD> &decomp_QTQ = decompositionCholesky(matrices.matQTQ, 5, 1);

    if (hasY) {
        // decomp_matB, decompQTQ sont indépendantes de la composante
        const t_prob hX = h_YWI_AY_composanteX(matrices, events, vecH, decomp_matB, decomp_QTQ, lambdaSpline);
        const t_prob hY = h_YWI_AY_composanteY(matrices, events, vecH, decomp_matB, decomp_QTQ, lambdaSpline);

        // To treat the 2D case, we use the 3D case by setting Yint = 100
        /*  const bool hasZ = (mCurveSettings.mProcessType == CurveSettings::eProcessType2D ||
                                           mCurveSettings.mProcessType == CurveSettings::eProcessTypeVector ||
                                           mCurveSettings.mProcessType == CurveSettings::eProcessTypeSpherical ||
                                           mCurveSettings.mProcessType == CurveSettings::eProcessType3D);

                        if (hasZ) {*/ //Always true
        const t_prob hZ = h_YWI_AY_composanteZ(matrices, events, vecH, decomp_matB, decomp_QTQ, lambdaSpline);
        return hX * hY *hZ;

    } else {
        return h_YWI_AY_composanteX(matrices, events, vecH, decomp_matB, decomp_QTQ, lambdaSpline);

    }

}


t_prob h_YWI_AY_composanteX(const SplineMatrices &matrices, const std::vector<std::shared_ptr<Event>> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, DiagonalMatrixLD> &decomp_matB, const std::pair<Matrix2D, DiagonalMatrixLD> &decomp_QTQ, const double lambdaSpline)
{
    if (lambdaSpline == 0) {
        return 1.;
    }

    const SplineResults &spline = doSplineX(matrices, events, vecH, decomp_matB, lambdaSpline);
    const std::vector< double> &vecG = spline.vecG;
    const DiagonalMatrixLD &matD = decomp_matB.second;

    // -------------------------------------------
    // Calcul de l'exposant
    // -------------------------------------------

    // Calcul de la forme quadratique YT W Y  et  YT WA Y

    const int n = (int)events.size();
    // Schoolbook algo
    /*
    t_prob h_exp = 0.;
    int i = 0;
    for (const auto& e : events) {
        h_exp  +=  e->mW * e->mYx * (e->mYx - vecG.at(i++));
    }
    */
    // C++ algo
    const t_prob h_exp = std::transform_reduce(PAR events.cbegin(), events.cend(), vecG.cbegin(), 0., std::plus{}, [](std::shared_ptr<Event> e,  double g) { return e->mW * e->mYx * (e->mYx - g); });

    // -------------------------------------------
    // Calcul de la norme
    // -------------------------------------------
    // Inutile de calculer le determinant de QT*Q (respectivement ST*Q)
    // (il suffit de passer par la décomposition Cholesky du produit matriciel QT*Q)
    // ni de calculer le determinant(Mat_B) car il suffit d'utiliser Mat_D (respectivement Mat_U) déjà calculé
    // inutile de refaire : Multi_Mat_par_Mat(Mat_QT,Mat_Q,Nb_noeuds,3,3,Mat_QtQ); -> déjà effectué dans calcul_mat_RQ

    const DiagonalMatrixLD &matDq = decomp_QTQ.second;

    // Schoolbook algo
    /*
    t_prob log_det_1_2 = 0.; // ne dépend pas de la composante X, Y ou Z
    for (int i = 1; i < n-1; ++i) { // correspond à i=shift jusqu'à nb_noeuds-shift
        log_det_1_2 += logl(matDq.at(i)/ matD.at(i));
    }
    */
    // C++ algo
    const t_prob log_det_1_2 = std::transform_reduce(PAR matDq.diagonal().cbegin()+1, matDq.diagonal().cend()-1, matD.diagonal().cbegin()+1, 0., std::plus{}, [](double val1,  double val2) { return logl(val1/val2); });

#ifdef DEBUG
#ifdef Q_OS_MAC
    if (math_errhandling & MATH_ERRNO) {
        if (errno==EDOM)
            qDebug()<<"[MCMCLoopCurve::h_YWI_AY_composanteX] errno set to EDOM";
    }
    if (math_errhandling  &MATH_ERREXCEPT) {
        if (fetestexcept(FE_INVALID))
            qDebug()<<"[MCMCLoopCurve::h_YWI_AY_composanteX] -> FE_INVALID raised : Domain error: At least one of the arguments is a value for which the function is not defined.";
    }
#endif
#endif
    // calcul à un facteur (2*PI) puissance -(n-2) près
    const t_prob res = 0.5 * ( (n -2.) * logl(lambdaSpline) + log_det_1_2 - h_exp) ;
    return exp(res) ;
}

t_prob h_YWI_AY_composanteY(const SplineMatrices &matrices, const std::vector<std::shared_ptr<Event>> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, DiagonalMatrixLD > &decomp_matB, const std::pair<Matrix2D, DiagonalMatrixLD> &decomp_QTQ, const double lambdaSpline)
{
    if (lambdaSpline == 0) {
        return 1.;
    }

    const SplineResults &spline = doSplineY(matrices, events, vecH, decomp_matB, lambdaSpline);
    const std::vector< double> &vecG = spline.vecG;
    const DiagonalMatrixLD &matD = decomp_matB.second;//.matD;
    // -------------------------------------------
    // Calcul de l'exposant
    // -------------------------------------------

    // Calcul de la forme quadratique YT W Y  et  YT WA Y

    const int n = (int)events.size();

    // Schoolbook algo
    /*
    t_prob h_exp = 0.;
    int i = 0;
    for (const auto& e : events) {
        h_exp  +=  e->mW * e->mYy * (e->mYy - vecG.at(i++));
    }
    */
    // C++ algo
    const t_prob h_exp = std::transform_reduce(PAR events.cbegin(), events.cend(), vecG.cbegin(), 0., std::plus{}, [](std::shared_ptr<Event> e,  double g) { return e->mW * e->mYy * (e->mYy - g); });

    // -------------------------------------------
    // Calcul de la norme
    // -------------------------------------------
    // Inutile de calculer le determinant de QT*Q (respectivement ST*Q)
    // (il suffit de passer par la décomposition Cholesky du produit matriciel QT*Q)
    // ni de calculer le determinant(Mat_B) car il suffit d'utiliser Mat_D (respectivement Mat_U) déjà calculé
    // inutile de refaire : Multi_Mat_par_Mat(Mat_QT,Mat_Q,Nb_noeuds,3,3,Mat_QtQ); -> déjà effectué dans calcul_mat_RQ

    const DiagonalMatrixLD& matDq = decomp_QTQ.second;
    /*
    t_prob log_det_1_2 = 0.;
    for (int i = 1; i < n-1; ++i) { // correspond à i=shift jusqu'à nb_noeuds-shift
        log_det_1_2 += logl(matDq.at(i)/ matD.at(i));
    }
    */
    // C++ algo
    const t_prob log_det_1_2 = std::transform_reduce(PAR matDq.diagonal().cbegin()+1, matDq.diagonal().cend()-1, matD.diagonal().cbegin()+1, 0., std::plus{}, [](double val1,  double val2) { return log(val1/val2); });

#ifdef DEBUG

#ifdef Q_OS_MAC
    if (math_errhandling & MATH_ERRNO) {
        if (errno==EDOM)
            qDebug()<<"[MCMCLoopCurve::h_YWI_AY_composanteY] errno set to EDOM";
    }
    if (math_errhandling  &MATH_ERREXCEPT) {
        if (fetestexcept(FE_INVALID))
            qDebug()<<"[MCMCLoopCurve::h_YWI_AY_composanteY] -> FE_INVALID raised : Domain error: At least one of the arguments is a value for which the function is not defined.";
    }
#endif
#endif
    // calcul à un facteur (2*PI) puissance -(n-2) près
    const t_prob res = 0.5 * ( (n - 2.) * logl(lambdaSpline) + log_det_1_2 - h_exp) ;
    return exp(res);
}

t_prob h_YWI_AY_composanteZ(const SplineMatrices &matrices, const std::vector<std::shared_ptr<Event>> &events,  const std::vector<t_reduceTime>& vecH, const std::pair<Matrix2D, DiagonalMatrixLD> &decomp_matB, const std::pair<Matrix2D, DiagonalMatrixLD> &decomp_QTQ, const double lambdaSpline)
{
    if (lambdaSpline == 0) {
        return 1.;
    }

    const SplineResults &spline = doSplineZ(matrices, events, vecH, decomp_matB, lambdaSpline);
    const std::vector< double> &vecG = spline.vecG;
    const DiagonalMatrixLD &matD = decomp_matB.second;//.matD;
    // -------------------------------------------
    // Calcul de l'exposant
    // -------------------------------------------

    // Calcul de la forme quadratique YT W Y  et  YT WA Y

    const int n = (int)events.size();

    /* Naive code
    t_prob h_exp = 0.;
    int i = 0;
    for (const auto& e : events) {
        h_exp  +=  e->mW * e->mYz * (e->mYz - vecG.at(i++));
    }
    */
    // C++ algo
    const t_prob h_exp = std::transform_reduce(PAR events.cbegin(), events.cend(), vecG.cbegin(), 0., std::plus{}, [](std::shared_ptr<Event> e,  double g) { return e->mW * e->mYz * (e->mYz - g); });

    // -------------------------------------------
    // Calcul de la norme
    // -------------------------------------------
    // Inutile de calculer le determinant de QT*Q (respectivement ST*Q)
    // (il suffit de passer par la décomposition Cholesky du produit matriciel QT*Q)
    // ni de calculer le determinant(Mat_B) car il suffit d'utiliser Mat_D (respectivement Mat_U) déjà calculé
    // inutile de refaire : Multi_Mat_par_Mat(Mat_QT,Mat_Q,Nb_noeuds,3,3,Mat_QtQ); -> déjà effectué dans calcul_mat_RQ

    const DiagonalMatrixLD& matDq = decomp_QTQ.second;

    /* Naive code
    t_prob log_det_1_2 = 0.;
    for (int i = 1; i < n-1; ++i) { // correspond à i=shift jusqu'à nb_noeuds-shift
        log_det_1_2 += logl(matDq.at(i)/ matD.at(i));
    }
    */
    // C++ algo
    const t_prob log_det_1_2 = std::transform_reduce(PAR matDq.diagonal().cbegin()+1, matDq.diagonal().cend()-1, matD.diagonal().cbegin()+1, 0.0, std::plus{}, [](double val1,  double val2) { return logl(val1/val2); });

#ifdef DEBUG
#ifdef Q_OS_MAC
    if (math_errhandling & MATH_ERRNO) {
        if (errno==EDOM)
            qDebug()<<"[MCMCLoopCurve::h_YWI_AY_composanteZ] errno set to EDOM";
    }
    if (math_errhandling  &MATH_ERREXCEPT) {
        if (fetestexcept(FE_INVALID))
            qDebug()<<"[MCMCLoopCurve::h_YWI_AY_composanteZ] -> FE_INVALID raised : Domain error: At least one of the arguments is a value for which the function is not defined.";
    }
#endif
#endif
    // calcul à un facteur (2*PI) puissance -(n-2) près
    const t_prob res = 0.5 * ( (n -2.) * logl(lambdaSpline) + log_det_1_2 - h_exp) ;
    return exp(res);
}

# pragma mark optimization



t_prob ln_h_YWI_3_update(const SplineMatrices &matrices, const std::vector<std::shared_ptr<Event> > &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, DiagonalMatrixLD> &decomp_matB, const double lambdaSpline, const bool hasY, const bool hasZ)
{
    Q_ASSERT_X(lambdaSpline!=0, "[ln_h_YWI_3_update]", "lambdaSpline = 0");
    const t_prob try_ln_h_YWI_3_X = ln_h_YWI_3_X(matrices, events, vecH, decomp_matB, lambdaSpline);

    const t_prob try_ln_h_YWI_3_Y = hasY ? ln_h_YWI_3_Y(matrices, events, vecH, decomp_matB, lambdaSpline) : 0.;

    const t_prob try_ln_h_YWI_3_Z = hasZ ? ln_h_YWI_3_Z( matrices, events, vecH, decomp_matB, lambdaSpline) : 0.;

    return try_ln_h_YWI_3_X + try_ln_h_YWI_3_Y + try_ln_h_YWI_3_Z;

}

t_prob ln_h_YWI_3_X(const SplineMatrices &matrices, const std::vector<std::shared_ptr<Event>> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, DiagonalMatrixLD> &decomp_matB, const double lambdaSpline)
{
    const SplineResults& spline = doSplineX(matrices, events, vecH, decomp_matB, lambdaSpline);

    const std::vector<double>& vecG = spline.vecG; //On peut utiliser do_vec_G

    // Calcul de la forme quadratique YT W Y  et  YT WA Y

    return -std::transform_reduce(PAR events.cbegin(), events.cend(), vecG.cbegin(), 0., std::plus{}, [](std::shared_ptr<Event> e,  double g) { return e->mW * e->mYx * (e->mYx - g); });

}

t_prob ln_h_YWI_3_Y(const SplineMatrices &matrices, const std::vector<std::shared_ptr<Event>> &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, DiagonalMatrixLD> &decomp_matB, const double lambdaSpline)
{
    const SplineResults &spline = doSplineY(matrices, events, vecH, decomp_matB, lambdaSpline);
    const std::vector<double> &vecG = spline.vecG;
    // Calcul de la forme quadratique YT W Y  et  YT WA Y

    return -std::transform_reduce(PAR events.cbegin(), events.cend(), vecG.cbegin(), 0., std::plus{}, [](std::shared_ptr<Event> e,  double g) { return e->mW * e->mYy * (e->mYy - g); });
}

t_prob ln_h_YWI_3_Z(const SplineMatrices &matrices, const std::vector<std::shared_ptr<Event> > &events,  const std::vector<t_reduceTime> &vecH, const std::pair<Matrix2D, DiagonalMatrixLD> &decomp_matB, const double lambdaSpline)
{
    const SplineResults &spline = doSplineZ(matrices, events, vecH, decomp_matB, lambdaSpline);
    const std::vector<double> &vecG = spline.vecG;

    // Calcul de la forme quadratique YT W Y  et  YT WA Y

    return -std::transform_reduce(PAR events.cbegin(), events.cend(), vecG.cbegin(), 0., std::plus{}, [](std::shared_ptr<Event> e,  double g) { return e->mW * e->mYz * (e->mYz - g); });
}


t_prob detPlus(const std::pair<Matrix2D, DiagonalMatrixLD > &decomp)
{
    return std::accumulate(decomp.second.diagonal().cbegin(), decomp.second.diagonal().cend(), 0., [](double prod, const double m){return prod + m;});
}



/**
* @brief Calculates the natural logarithm of the determinant of a matrix's diagonal elements
*
* @details This function computes:
* @f[
* \ln(\det^+) = \sum_{i=1}^{n-2} \ln(m_i)
* @f]
* where @f$m_i@f$ are the diagonal elements of the matrix (excluding first and last elements).
* The "+" in @f$\det^+@f$ indicates that we're using a modified determinant calculation
* that skips the first and last diagonal elements.
*
* @param decomp A pair containing:
*        - First: A 2D matrix (Matrix2D)
*        - Second: A diagonal matrix (MatrixDiag) whose logarithmic determinant we calculate
*
* @return t_prob The sum of the natural logarithms of the diagonal elements
*
* @note The function skips the first and last elements of the diagonal matrix
* @note Uses parallel reduction for large matrices (> 1000 elements)
*/
t_prob ln_detPlus(const std::pair<Matrix2D, DiagonalMatrixLD>& decomp)
{
    const auto& diag = decomp.second.diagonal();
    const size_t size = diag.rows();

    // Handle edge cases , yet tested before
    // if (size <= 2) return 0.0;

    t_prob sum = 0.0;
    // Pour les petites matrices, utiliser la version séquentielle
    if (size < 1000) {
        for (size_t i = 1; i < size - 1; ++i) {
            sum += std::log(diag[i]);
        }
        return sum;
    }

    // Pour les grandes matrices, utiliser la parallélisation
#pragma omp parallel reduction(+:sum)
    {
#pragma omp for nowait
        for (size_t i = 1; i < size - 1; ++i) {
            sum += std::log(diag[i]);
        }
    }

    return sum;
}

t_prob ln_h_YWI_1(const std::pair<Matrix2D, DiagonalMatrixLD> &decomp_QTQ)
{
    return ln_detPlus(decomp_QTQ);
}

t_prob ln_h_YWI_2(const std::pair<Matrix2D, DiagonalMatrixLD> &decomp_matB)
{
    return -ln_detPlus(decomp_matB);
}

t_prob ln_h_YWI_1_2(const std::pair<Matrix2D, DiagonalMatrixLD>& decomp_QTQ, const std::pair<Matrix2D, DiagonalMatrixLD >& decomp_matB)
{
    const auto& diagDq = decomp_QTQ.second.diagonal();  // Eigen::Matrix<t_matrix, Dynamic, 1>
    const auto& diagD  = decomp_matB.second.diagonal();   // même type

    return std::transform_reduce(PAR diagDq.cbegin()+1, diagDq.cend()-1, diagD.cbegin()+1, 0., std::plus{}, [](double val1,  double val2) { return std::log(val1 / val2); });

}

/* version school Book
t_prob ln_rate_det_B (const std::pair<Matrix2D, DiagonalMatrixLD>& try_decomp_B, const std::pair<Matrix2D, DiagonalMatrixLD >& current_decomp_B)
{
    const auto& try_B_diag = try_decomp_B.second.diagonal();  // Eigen::Matrix<t_matrix, Dynamic, 1>
    const auto& current_B_diag  = current_decomp_B.second.diagonal();   // même type

    return std::transform_reduce(PAR try_B_diag.cbegin()+1, try_B_diag.cend()-1, current_B_diag.cbegin()+1, 0., std::plus{}, [](double B_new,  double B_old) { return std::log(B_old / B_new); });

}
*/

t_prob ln_rate_det_B(const std::pair<Matrix2D, DiagonalMatrixLD>& try_decomp_B,
                     const std::pair<Matrix2D, DiagonalMatrixLD>& current_decomp_B)
{
    // pour le rapport de det B; le ratio est inversé B_old/B_new
    return log_det_ratio(current_decomp_B.second.diagonal(), try_decomp_B.second.diagonal());
}

/* version school Book
t_prob ln_rate_det_QtQ_det_B (const std::pair<Matrix2D, DiagonalMatrixLD>& try_decomp_QtQ, const std::pair<Matrix2D, DiagonalMatrixLD >& try_decomp_B,
                             const std::pair<Matrix2D, DiagonalMatrixLD>& current_decomp_QtQ, const std::pair<Matrix2D, DiagonalMatrixLD >& current_decomp_B)
{
    const auto& try_QtQ_diag = try_decomp_QtQ.second.diagonal();  // Eigen::Matrix<t_matrix, Dynamic, 1>
    const auto& try_B_diag  = try_decomp_B.second.diagonal();   // même type
    const auto& current_QtQ_diag = current_decomp_QtQ.second.diagonal();  // Eigen::Matrix<t_matrix, Dynamic, 1>
    const auto& current_B_diag  = current_decomp_B.second.diagonal();   // même type

    t_prob sum_QtQ = std::transform_reduce(PAR try_QtQ_diag.cbegin()+1, try_QtQ_diag.cend()-1, current_QtQ_diag.cbegin()+1, 0., std::plus{}, [](long double QtQ_new, long double QtQ_old) { return std::log(QtQ_new / QtQ_old); });
    t_prob sum_B =  std::transform_reduce(PAR try_B_diag.cbegin()+1, try_B_diag.cend()-1, current_B_diag.cbegin()+1, 0., std::plus{}, [](long double B_new, long double B_old) { return std::log(B_old / B_new); });

    return sum_QtQ + sum_B;
}
*/

// version utilisant eigen
t_prob ln_rate_det_QtQ_det_B(const std::pair<Matrix2D, DiagonalMatrixLD>& try_decomp_QtQ,
                             const std::pair<Matrix2D, DiagonalMatrixLD>& try_decomp_B,
                             const std::pair<Matrix2D, DiagonalMatrixLD>& current_decomp_QtQ,
                             const std::pair<Matrix2D, DiagonalMatrixLD>& current_decomp_B)
{
    return log_det_ratio(try_decomp_QtQ.second.diagonal(), current_decomp_QtQ.second.diagonal())
           + log_det_ratio(current_decomp_B.second.diagonal(), try_decomp_B.second.diagonal());
}

