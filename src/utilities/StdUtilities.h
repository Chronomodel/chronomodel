/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2026

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

#ifndef STDUTILITIES_H
#define STDUTILITIES_H

#pragma once

#include <RefCurve.h>

#include <chrono>
#include <valarray>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>
#include <math.h>
#include <iterator>

#include "fftw3.h" // pour UCV

#include <QMap>
#include <QList>
#include <QList>
#include <QDebug>


#ifndef M_PI
#define M_PI 3.14159265358979323846 //usefull to Windows
#endif

#ifndef M_PIl
#define M_PIl 3.14159265358979323846264338327950288419716939937510L //usefull in long double
#endif

#ifndef M_SQRT2PI
#define M_SQRT2PI 2.5066282746310005024157652848110452530069867406099383166299235763 //sqrt(2*pi)
#endif




typedef QString (*FormatFunc)(const double, const bool forcePrecision);

const double gammaActivity[] = {1.0,-1.0,0.00501257,0.0589032,0.140868,0.222072,0.294314,0.356635,0.410058,0.455966,0.495647,0.530184,0.560456,0.587174,
                                0.610905,0.632109,0.651162,0.668367,0.683976,0.698199,0.71121,0.723155,0.73416,0.744329,0.753754,0.762514,0.770675,0.778297,
                                0.785431,0.792122,0.79841,0.804331,0.809914,0.81519,0.820181,0.82491,0.829398,0.833662,0.837718,0.841582,0.845267,0.848784,
                                0.852146,0.855361,0.85844,0.861391,0.864222,0.866939,0.86955,0.87206,0.874476,0.876803,0.879045,0.881207,0.883293,0.885307,
                                0.887253,0.889134,0.890953,0.892714,0.894418,0.89607,0.897671,0.899223,0.900729,0.90219,0.903609,0.904988,0.906327,0.90763,
                                0.908897,0.910129,0.911329,0.912497,0.913635,0.914743,0.915824,0.916877,0.917905,0.918907,0.919885,0.92084,0.921772,0.922683,
                                0.923573,0.924442,0.925292,0.926123,0.926936,0.927731,0.928509,0.92927,0.930016,0.930746,0.93146,0.932161,0.932846,0.933519,
                                0.934178,0.934824,0.935457,0.936079,0.936688,0.937286,0.937873,0.938449,0.939014,0.939569,0.940114,0.940649,0.941175,0.941691,
                                0.942199,0.942698,0.943188,0.94367,0.944144,0.94461,0.945068,0.945519,0.945962,0.946398,0.946828,0.94725,0.947666,0.948075,
                                0.948478,0.948875,0.949265,0.94965,0.950029,0.950402,0.95077,0.951132,0.95149,0.951841,0.952188,0.95253,0.952867,0.953199, 0.953527,0.95385,0.954168,0.954483,0.954793,0.955098,0.9554,0.955697,0.955991,0.956281,0.956567,0.956849,0.957128,0.957403, 0.957674,0.957943,0.958207,0.958469,0.958727,0.958982,0.959234,0.959483,0.959728,0.959971,0.960211,0.960448,0.960682,0.960914,  0.961143,0.961369,0.961592,0.961813,0.962031,0.962247,0.962461,0.962672,0.962881,0.963087,0.963291,0.963493,0.963693,0.96389,  0.964085,0.964279,0.96447,0.964659,0.964846,0.965031,0.965214,0.965396,0.965575,0.965753,0.965929,0.966103,0.966275,0.966445, 0.966614,0.966781,0.966946,0.96711,0.967272,0.967433,0.967592,0.967749,0.967905,0.968059,0.968212,0.968364,0.968514,0.968663,  0.96881,0.968956,0.9691,0.969243,0.969385,0.969526,0.969665,0.969803,0.96994,0.970075,0.970209,0.970343,0.970474,0.970605, 0.970735,0.970863,0.97099,0.971117,0.971242,0.971366,0.971489,0.971611,0.971732,0.971852,0.97197,0.972088,0.972205,0.972321, 0.972436,0.97255,0.972663,0.972775,0.972886,0.972996,0.973106,0.973214,0.973322,0.973429,0.973535,0.97364,0.973744,0.973847, 0.97395,0.974052,0.974153,0.974253,0.974353,0.974451,0.974549,0.974647,0.974743,0.974839,0.974934,0.975028,0.975122,0.975214, 0.975307,0.975398,0.975489,0.975579,0.975669,0.975758,0.975846,0.975933,0.97602,0.976106,0.976192,0.976277,0.976362,0.976446,  0.976529,0.976612,0.976694,0.976775,0.976856,0.976937,0.977017,0.977096,0.977174,0.977253,0.97733,0.977408,0.977484,0.97756,  0.977636,0.977711,0.977786,0.97786,0.977933,0.978006,0.978079,0.978151,0.978223,0.978294,0.978365,0.978435,0.978505,0.978574, 0.978643,0.978712,0.97878,0.978847,0.978914,0.978981,0.979048,0.979114,0.979179,0.979244,0.979309,0.979373,0.979437,0.9795,0.979564, 0.979626,0.979689,0.979751,0.979812,0.979873,0.979934,0.979995,0.980055,0.980115,0.980174,0.980233,0.980292,0.98035,0.980408, 0.980466,0.980523,0.98058,0.980637,0.980693,0.980749,0.980805,0.98086,0.980915,0.98097,0.981024,0.981078,0.981132,0.981185,0.981239, 0.981291,0.981344,0.981396,0.981448,0.9815,0.981551,0.981602,0.981653,0.981704,0.981754,0.981804,0.981854,0.981903,0.981953,  0.982001,0.98205,0.982099,0.982147,0.982195,0.982242,0.98229,0.982337,0.982384,0.98243,0.982477,0.982523,0.982569,0.982614,0.98266, 0.982705,0.98275,0.982794,0.982839,0.982883,0.982927,0.982971,0.983015,0.983058,0.983101,0.983144,0.983187,0.983229,0.983271,0.983314,0.983355,0.983397,0.983438,0.98348,0.983521,0.983561,0.983602,0.983642,0.983683,0.983723,0.983762,0.983802,0.983841,0.983881,0.98392,0.983959,0.983997,0.984036,0.984074,0.984112,0.98415,0.984188,0.984225,0.984263,0.9843,0.984337,0.984374,0.984411,0.984447,0.984484,0.98452,0.984556,0.984592,0.984627,0.984663,0.984698,0.984733,0.984768,0.984803,0.984838,0.984873,0.984907,0.984941,0.984975,0.985009,0.985043,0.985076,0.98511,0.985143,0.985177,0.98521,0.985242,0.985275,0.985308,0.98534,0.985372,0.985405,0.985437,0.985469,0.9855,0.985532,0.985563,0.985595,0.985626,0.985657,0.985688,0.985719,0.985749,0.98578,0.98581,0.98584,0.985871,0.985901,0.985931,0.98596,0.98599,0.986019,0.986049,0.986078,0.986107,0.986136,0.986165,0.986194,0.986223,0.986251,0.986279,0.986308,0.986336,0.986364,0.986392,0.98642,0.986448,0.986475,0.986503,0.98653,0.986557,0.986584,0.986612,0.986638,0.986665,0.986692,0.986719,0.986745,0.986772};

int compareStrings(const std::string &s1, const std::string &s2);
std::string removeZeroAtRight(std::string str);

double safeExp(const double x, int n = 10);
double safeLog(const double x, int n = 5);

void checkFloatingPointException(const QString &infos = QString());


QList<double> normalize_vector(const QList<double> &vector);
QList<float> normalize_vector(const QList<float> &vector);

QMap<double, double> equal_areas(const QMap<double, double> &mapToModify, const QMap<double, double> &mapWithTargetArea);
QMap<float, float> equal_areas(const QMap<float, float> &mapToModify, const QMap<float, float> &mapWithTargetArea);
std::map<double, double> equal_areas(const std::map<double, double>& mapToModify, const std::map<double, double>& mapWithTargetArea);

QMap<double, double> equal_areas(const QMap<double, double> &mapToModify, const double targetArea);
QMap<float, float> equal_areas(const QMap<float, float> &mapToModify, const float targetArea);
std::map<double, double> equal_areas(const std::map<double, double> &mapToModify, const double targetArea);

QList<double> equal_areas(const QList<double> &data, const double step, const double area);
QList<float> equal_areas(const QList<float> &data, const float step, const float area);
std::vector<double> equal_areas(const std::vector<double>& data, const float step, const float area);


QMap<float, float> vector_to_map(const QList<float> &data, const float min, const float max, const float step);
QMap<double, double> vector_to_map(const QList<double> &data, const double min, const double max, const double step);
QMap<double, double> vector_to_map(const QList<int> &data, const double min, const double max, const double step);

std::map<double, double> vector_to_map(const std::vector<double> &data, const double min, const double max, const double step);

//There is also RefCurve::interpolate_mean()
/**
 * @brief Interpolates a value from a uniformly sampled curve, with
 *        special handling for flat plateaus.
 *
 * The curve is assumed to be sampled uniformly between @p curveTmin
 * (index 0) and @p curveTmax (index @c curve.size()‑1).  If the two
 * neighbour points that surround the requested time have (almost) the
 * same value, the function returns that value directly – no interpolation
 * is performed across a plateau edge.
 *
 * @param t          Time for which the value is required.
 * @param curve      Vector (or QList) containing the sampled values.
 * @param curveTmin  Time that corresponds to curve[0].
 * @param curveTmax  Time that corresponds to curve.back().
 * @param eps        Tolerance used to decide whether two neighbour values
 *                   belong to the same plateau (default = 1E‑12).
 *
 * @return Interpolated value, or the plateau value when appropriate.
 */
inline double interpolate_value_from_curve(double t,
                                           const QList<double> &curve,
                                           double curveTmin,
                                           double curveTmax,
                                           double eps = 1E-12) noexcept
{
    const std::size_t N = curve.size();
    // --------------------------------------------------------------
    // 1️⃣  Guard against degenerate input
    // --------------------------------------------------------------
    if (N < 2) return 0.0;

    // --------------------------------------------------------------
    // 2️⃣  Fast handling of the two extremes
    // --------------------------------------------------------------
    if (t <= curveTmin) return curve.front();
    if (t >= curveTmax) return curve.back();

    // --------------------------------------------------------------
    // 3️⃣  Compute the (floating‑point) index in the array
    // --------------------------------------------------------------

    const double step = (curveTmax - curveTmin) / static_cast<double>(N - 1);
    const double idx  = (t - curveTmin) / step;          // idx ∈ [0, N‑1]

    const std::size_t i = static_cast<std::size_t>(idx); // floor
    const double frac   = idx - static_cast<double>(i);   // fractional part

    // --------------------------------------------------------------
    // 4️⃣ Retrieve the two neighbour values
    // --------------------------------------------------------------
    const double v0 = curve[i];
    const double v1 = curve[i + 1];

    // --------------------------------------------------------------
    // 5️⃣ Plateau detection (values equal within eps)
    // --------------------------------------------------------------
    if (std::fabs(v0 - v1) <= eps) return v0;            // plateau

    // --------------------------------------------------------------
    // 6️⃣ Handle the “zero‑value” special case (if you really need it)
    // --------------------------------------------------------------
    if (v0 == 0.0 && v1 == 0.0) {
        // Both are zero → the whole segment is effectively a zero plateau.
        return 0.0;
    }

    // --------------------------------------------------------------
    // 7️⃣ Exact hit → no interpolation needed
    // --------------------------------------------------------------
    // one side is zero, return the other
    if (v0 == 0.0) return v1;
    if (v1 == 0.0) return v0;

    // --------------------------------------------------------------
    // 8️⃣  Normal linear interpolation (no plateau)
    // --------------------------------------------------------------
    return v0 + frac * (v1 - v0);                        // linear interpolation
}
/**
 * @brief Interpolates a value from a uniformly sampled curve.
 *
 * The curve is sampled uniformly between @p Xmin (index 0) and @p Xmax
 * (index @c curve.size()‑1).  The function returns the value that
 * corresponds to the requested abscissa @p x.
 *
 * Special handling:
 *   • If @p x lies outside the interval, the nearest endpoint is returned.
 *   • If the two neighbour points are (almost) equal, the plateau value
 *     is returned – no interpolation is performed across a flat region.
 *   • If the upper neighbour is exactly 0, the function returns 0 (the
 *     original “gate” rule).  If only the lower neighbour is 0, the
 *     non‑zero neighbour is returned.
 *
 * @param[in] x        The abscissa for which a value is required.
 * @param[in] curve    Vector containing the sampled values (must contain
 *                     at least two points).
 * @param[in] Xmin     Abscissa that corresponds to @c curve[0].
 * @param[in] Xmax     Abscissa that corresponds to @c curve.back().
 * @param[in] eps      Tolerance used to decide whether two neighbour values
 *                     belong to the same plateau (default = 1E‑12).
 *
 * @return Interpolated value (or the nearest endpoint / plateau value).
 *
 * @note The implementation is `inline` and `noexcept` and performs only
 *       a single division, two memory accesses and one multiplication per call.
 */
inline double interpolate_value_from_curve(double x,
                                           const std::vector<double>& curve,
                                           double Xmin,
                                           double Xmax,
                                           double eps = 1E-12) noexcept
{
    const std::size_t N = curve.size();

    // --------------------------------------------------------------
    // 1️⃣  Guard against degenerate input
    // --------------------------------------------------------------
    if (N < 2) return 0.0;                 // should never happen – defensive

    // --------------------------------------------------------------
    // 2️⃣  Fast handling of the two extremes
    // --------------------------------------------------------------
    if (x <= Xmin) return curve.front();
    if (x >= Xmax) return curve.back();

    // --------------------------------------------------------------
    // 3️⃣  Compute the step (distance between two consecutive samples)
    // --------------------------------------------------------------
    const double step = (Xmax - Xmin) / static_cast<double>(N - 1);

    // --------------------------------------------------------------
    // 4️⃣  Floating‑point index and its integer / fractional parts
    // --------------------------------------------------------------
    const double idxReal = (x - Xmin) / step;          // idx ∈ [0, N‑1]
    const std::size_t i   = static_cast<std::size_t>(idxReal); // floor
    const double frac     = idxReal - static_cast<double>(i);   // fractional part

    // i is guaranteed to be < N‑1 because x < Xmax (handled above)
    const double v0 = curve[i];
    const double v1 = curve[i + 1];

    // --------------------------------------------------------------
    // 5️⃣  Plateau detection (values equal within eps)
    // --------------------------------------------------------------
    if (std::fabs(v0 - v1) <= eps) {
        // Both points belong to the same flat region → return that value.
        return v0;                     // v0 == v1 within tolerance
    }

    // --------------------------------------------------------------
    // 6️⃣  Original “gate” rule – avoid interpolation when the upper
    //     neighbour is exactly zero.
    // --------------------------------------------------------------
    if (v1 == 0.0) {
        // Upper point is a gate → return 0 (the original behaviour).
        return 0.0;
    }

    // If the lower point is zero but the upper one is not, we simply
    // return the upper value – this is mathematically equivalent to a
    // linear interpolation where one endpoint is zero.
    if (v0 == 0.0) return v1;

    // --------------------------------------------------------------
    // 7️⃣  Normal linear interpolation (no plateau, both points non‑zero)
    // --------------------------------------------------------------
    return v0 + frac * (v1 - v0);
}



float map_area(const QMap<float, float> &map);
double map_area(const QMap<int, double> &density);

double map_area(const std::map<double, double> &map);

inline double map_area(const QMap<double, double> &map)
{
    return map_area(map.toStdMap());
}

inline double surface_on_theta (std::map<double, double>::const_iterator iter_on_theta );

const std::map<double, double> create_HPD2(const QMap<double, double> &density, const double threshold = 95.);// useless

const std::map<double, double> create_HPD_mapping(const QMap<double, double> &density, std::map<double, double> &area_mapping, const double threshold = 95.);

//const std::map<double, double> create_HPD_by_dichotomy(const QMap<double, double> &density, QList<QPair<double, QPair<double, double> > > &intervals_hpd, const double threshold);
const std::map<double, double> create_HPD_by_dichotomy(const std::map<double, double> &density, QList<QPair<double, QPair<double, double> > > &intervals_hpd, const double threshold);

inline const std::map<double, double> create_HPD_by_dichotomy(const QMap<double, double> &density, QList<QPair<double, QPair<double, double> > > &intervals_hpd, const double threshold)
{
    return create_HPD_by_dichotomy(density.toStdMap(), intervals_hpd, threshold);
}

QList<double> vector_to_histo(const QList<double> &vector, const double tmin, const double tmax, const int nbPts);

std::valarray<double> polynom_regression_coef(QMap<double, double> &data, int d);
double MSE(const QMap<double, double> &data,  const std::valarray<double> polynom_coef);
double Pearson_X_square(const QMap<double, double> &data,  const std::valarray<double> polynom_coef);

#pragma mark Template Function

template <typename T, typename V>
inline V interpolate(const T& x, const T& x0, const T& x1, const V& y0, const V& y1)
{
    Q_ASSERT(x0!=x1);
    //return (y1 + (y2 - y1) * V((x - x1) / (x2 - x1)) ); // schoolbook code

    return std::lerp(y0, y1, V((x - x0) / (x1 - x0)));
}

/**
 * @brief This only works for strictly increasing functions!
 * @return interpolated index for a given value. If the value is smaller than all vector values, 0 is returned. If the value is greater than all vector values, (vector.size() - 1) is returned.
 */
// remplacer par interpolate_index en dessous
template <template<typename...> class Container, class T >
T vector_interpolate_idx_for_value(const T value, const Container<T> &vector, decltype(vector.size()) idxInf = 0, decltype(vector.size()) idxSup = 0)
{
    if (idxSup == 0)
        idxSup = vector.size() - 1;

    if (value < vector.front())
        return T (0);

    if  (value > vector.back())
        return  T (vector.size() - 1);

    // Dichotomie, we can't use indexOf because we don't know the step between each value in the QList

    if (idxSup > idxInf) {
        do {
            const decltype(vector.size()) idxMid = idxInf + (idxSup - idxInf) / 2;
            const T valueMid = vector.at(idxMid);

            if (value < valueMid)
                idxSup = idxMid;
            else
                idxInf = idxMid;

        } while (idxSup - idxInf > 1);

        // Interpolation linéaire
        T valueInf = vector.at(idxInf);
        T valueSup = vector.at(idxSup);

        // Gestion des cas spéciaux avec tolérance numérique
        const T epsilon = static_cast<T>(1e-300);

        // test si on a atteind la valeur exacte
        if (std::abs(valueInf - value) <= epsilon) {
            valueSup = value;
            idxSup = idxInf;

        } else  if (std::abs(valueSup - value) <= epsilon) {
            valueInf = value;
            idxInf = valueSup;
        }

        T idx;
        if (std::abs(valueSup - valueInf) <= epsilon) { // on recherche la taille du plateau, on élargie pour determiner le centre
            // Élargir vers la gauche
            while (idxInf>0 && std::abs(valueInf - vector.at(idxInf-1)) <= epsilon) {
                idxInf -= 1;
                valueInf = vector.at(idxInf);
            };
            // Élargir vers la droite
            while (idxSup < vector.size()-1 && std::abs(valueSup - vector.at(idxSup+1)) <= epsilon) {
                idxSup += 1;
                valueSup = vector.at(idxSup);
            };

            idx = T((idxSup+idxInf)/2);

        } else {
            // On ressert l'intervale, une des bornes est sur le plateau
            while (idxInf>0 && std::abs(valueInf - vector.at(idxInf+1)) <= epsilon) {
                idxInf += 1;
                valueInf = vector.at(idxInf);
            };
            // Vérifier si la borne supérieure est sur un plateau
            while (idxSup<vector.size()-1 && std::abs(valueSup - vector.at(idxSup-1)) <= epsilon) {
                idxSup -= 1;
                valueSup = vector.at(idxSup);
            };


            T prop = 0.;
            // prevent valueSup=valueInf because in this case prop = NaN
            if (valueSup > valueInf)
                prop = (value - valueInf) / (valueSup - valueInf);

            idx =  idx = prop * T(idxSup - idxInf) + T(idxInf);

        }
        return idx;

    } else if (idxSup == idxInf) {
        return T (idxInf);
    }

    return T (0);
}


/**
 * @brief Interpolation linéaire d’un indice dans un tableau strictement croissant.
 *
 * La fonction accepte tout conteneur qui expose :
 *   - size()
 *   - front() / back()
 *   - operator[](std::size_t)
 *
 * @tparam Container   Conteneur (QVector<T>, QList<T>, std::vector<T>, Eigen::VectorXd …)
 * @tparam ValueT      Type des valeurs stockées (float, double, long double …)
 * @tparam IndexT      Type du résultat (double, long double, float, std::size_t …)
 *
 * @param value        Valeur à interpoler.
 * @param data         Conteneur de valeurs strictement croissantes.
 * @param idxInfStart  Indice inférieur de départ (optionnel, 0 par défaut).
 * @param idxSupStart  Indice supérieur de départ (optionnel, size‑1 par défaut).
 *
 * @return Un indice (type @p IndexT) où la valeur serait située.
 *         Si la valeur est hors du domaine, on renvoie 0 ou size‑1.
 */
/*
template <class Container,
         class ValueT = typename Container::value_type,
         class IndexT = double>
IndexT interpolate_index(const ValueT& value,
                         const Container& data,
                         std::size_t idxInfStart = 0,
                         std::size_t idxSupStart = static_cast<std::size_t>(-1))
{
    static_assert(std::is_floating_point<ValueT>::value,
                  "ValueT must be a floating‑point type");
    static_assert(std::is_arithmetic<IndexT>::value,
                  "IndexT must be an arithmetic type");

    const std::size_t n = data.size();
    assert(n > 0 && "Container must not be empty");

    // -----------------------------------------------------------------
    // Gestion des bornes du domaine
    // -----------------------------------------------------------------
    if (value <= data.front())
        return static_cast<IndexT>(0);
    if (value >= data.back())
        return static_cast<IndexT>(n - 1);

    // -----------------------------------------------------------------
    // Initialisation des indices de recherche
    // -----------------------------------------------------------------
    std::size_t idxInf = idxInfStart;
    std::size_t idxSup = (idxSupStart == static_cast<std::size_t>(-1))
                             ? n - 1
                             : idxSupStart;

    // -----------------------------------------------------------------
    // Recherche binaire (dichotomie)
    // -----------------------------------------------------------------
    while (idxSup - idxInf > 1) {
        const std::size_t idxMid = idxInf + (idxSup - idxInf) / 2;
        const ValueT      valMid = data[idxMid];

        if (value < valMid) {
            idxSup = idxMid;
        } else if (value > valMid) {
            idxInf = idxMid;
        } else {                     // valeur exactement égale à valMid
            idxInf = idxMid;
            idxSup = idxMid;
            break;
        }
    }

    // -----------------------------------------------------------------
    // Gestion du plateau (valeurs identiques à cause de la précision)
    // -----------------------------------------------------------------
    // epsilon adapté à la précision du type flottant
    const ValueT eps = std::numeric_limits<ValueT>::epsilon() *
                       std::max<ValueT>({std::abs(value),
                                         std::abs(data[idxInf]),
                                         std::abs(data[idxSup])});

    // Étendre à gauche tant que les valeurs sont « égales »
    std::size_t left = idxInf;
    while (left > 0 && std::abs(data[left] - data[left - 1]) <= eps)
        --left;

    // Étendre à droite tant que les valeurs sont « égales »
    std::size_t right = idxSup;
    while (right + 1 < n && std::abs(data[right] - data[right + 1]) <= eps)
        ++right;

    // Si tout le plateau couvre la zone recherchée, on renvoie son centre
    if (std::abs(data[left] - data[right]) <= eps) {
        return static_cast<IndexT>(left + right) / static_cast<IndexT>(2);
    }

    // -----------------------------------------------------------------
    // Interpolation linéaire entre les bords du plateau
    // -----------------------------------------------------------------
    const ValueT vL = data[left];
    const ValueT vR = data[right];

    // t ∈ [0,1] représente la position relative de «value» entre vL et vR
    const ValueT t = (value - vL) / (vR - vL);

    // Calcul de l’indice réel (peut être non entier)
    const IndexT idx = static_cast<IndexT>(left) +
                       static_cast<IndexT>(t) * static_cast<IndexT>(right - left);

    return idx;
}
*/

template <class Container,
         class ValueT = typename Container::value_type,
         class IndexT = double>
IndexT interpolate_index(const ValueT& value,
                         const Container& data,
                         std::size_t idxInfStart = 0,
                         std::size_t idxSupStart = static_cast<std::size_t>(-1))
{
    static_assert(std::is_floating_point<ValueT>::value,
                  "ValueT must be a floating‑point type");
    static_assert(std::is_arithmetic<IndexT>::value,
                  "IndexT must be an arithmetic type");

    const std::size_t n = data.size();
    assert(n > 0 && "Container must not be empty");

    // -----------------------------------------------------------------
    // Gestion des bornes du domaine
    // -----------------------------------------------------------------
    if (value <= data.front())
        return static_cast<IndexT>(0);
    if (value >= data.back())
        return static_cast<IndexT>(n - 1);

    // -----------------------------------------------------------------
    // Initialisation des indices de recherche
    // -----------------------------------------------------------------
    std::size_t idxInf = idxInfStart;
    std::size_t idxSup = (idxSupStart == static_cast<std::size_t>(-1))
                             ? n - 1
                             : idxSupStart;

    // -----------------------------------------------------------------
    // Recherche binaire (dichotomie)
    // -----------------------------------------------------------------
    while (idxSup - idxInf > 1) {
        const std::size_t idxMid = idxInf + (idxSup - idxInf) / 2;
        const ValueT      valMid = data[idxMid];

        if (value < valMid) {
            idxSup = idxMid;
        } else if (value > valMid) {
            idxInf = idxMid;
        } else {                     // valeur exactement égale à valMid
            idxInf = idxMid;
            idxSup = idxMid;
            break;
        }
    }

    // -----------------------------------------------------------------
    // Epsilon adapté à la précision du type flottant
    // -----------------------------------------------------------------
    const ValueT eps = std::numeric_limits<ValueT>::epsilon() *
                       std::max<ValueT>({std::abs(value),
                                         std::abs(data[idxInf]),
                                         std::abs(data[idxSup])});

    // -----------------------------------------------------------------
    // Avancer idxInf jusqu'au bord droit de son plateau
    // (la dichotomie peut laisser idxInf au milieu d'une zone plate)
    // La dichotomie garantit déjà data[idxSup] > value, il n'y a donc
    // pas besoin de corriger idxSup.
    // -----------------------------------------------------------------
    while (idxInf + 1 < idxSup &&
           std::abs(data[idxInf + 1] - data[idxInf]) <= eps)
        ++idxInf;

    // -----------------------------------------------------------------
    // Interpolation linéaire
    // -----------------------------------------------------------------
    const ValueT vL = data[idxInf];
    const ValueT vR = data[idxSup];

    // Sécurité : si les deux bornes sont identiques (plateau total)
    // on renvoie le centre
    if (std::abs(vR - vL) <= eps)
        return static_cast<IndexT>(idxInf + idxSup) / static_cast<IndexT>(2);

    // t ∈ [0,1] : position relative de value entre vL et vR
    const ValueT t = (value - vL) / (vR - vL);

    // Indice réel interpolé
    const IndexT idx = static_cast<IndexT>(idxInf) +
                       static_cast<IndexT>(t) *
                           static_cast<IndexT>(idxSup - idxInf);

    return idx;
}

template <typename T, typename U>
T interpolateValueInQMap(const U &key, const QMap<U, T> &map)
{
    if (key <= map.firstKey()) {
        return map.first();

    } else if (key >= map.lastKey()) {
        return map.last();

    } else {
        const auto uIter =  map.upperBound(key) ;
        const auto lIter = std::prev(uIter);

        return interpolate(key, lIter.key(), uIter.key(), lIter.value(), uIter.value());
    }

}
template <typename T, typename U>
T interpolateValueInStdMap(const U &key, const std::map<U, T> &map)
{
    if (key <= map.begin()->first) {
        return map.begin()->second;

    } else if (key >= map.crbegin()->first) {
        return map.crbegin()->second;

    } else {
        const auto uIter =  map.upper_bound(key) ;
        const auto lIter = std::prev(uIter);

        return interpolate(key, lIter->first, uIter->first, lIter->second, uIter->second);
    }
}

template <template<typename...> class C, class T>
T range_max_value(const C<T> &range)
{
    return *std::ranges::max_element(range.begin(), range.end());
}

template <template<typename...> class C, class T>
T range_min_value(const C<T>& range)
{
    return *std::ranges::min_element(range.begin(), range.end());
}


template <class U, class T>
typename QMap<U, T>::const_iterator map_max(const QMap<U, T> &map)
{
    /** return std::max_element(map.toStdMap().begin(), map.toStdMap().end(),
                              [](const QMap<U, T>& p1, const QMap<U, T>& p2) {
                                  return p1.values() < p2.values(); });
    */
    typename QMap<U, T>::const_iterator i = map.cbegin();
    typename QMap<U, T>::const_iterator biggest = i;
    ++i;

    for (; i != map.cend(); ++i)
        if (*i > *biggest )  biggest = i;

    return biggest;

}
template <class U, class T>
typename std::map<U, T>::const_iterator map_max(const std::map<U, T> &map)
{
    typename std::map<U, T>::const_iterator i = map.cbegin();
    typename std::map<U, T>::const_iterator biggest = i;
    ++i;

    for (; i != map.cend(); ++i)
        if (i->second > biggest->second )  biggest = i;

    return biggest;

}
/**
 * @brief We assume that min and max are in the values of the map
 * @param map
 * @param min
 * @param max
 * @return a pointer to the element with the largest value
 */
template <class U, class T>
typename QMap<U, T>::const_iterator map_max(const QMap<U, T> &map, U min, U max)
{
    typename QMap<U, T>::const_iterator i = map.cbegin();
    if (map.lastKey()<min)
        return i;

    else if (map.firstKey()>max)
        return i;

    else {
        while (i.key()<min)
            ++i;

        typename QMap<U, T>::const_iterator biggest = i;
        for (; i != map.cend() && i.key()<=max; ++i)
            if (*i > *biggest )  biggest = i;

        return biggest;
    }

}

template <class U, class T>
typename std::map<U, T>::const_iterator map_max(const std::map<U, T> &map, U min, U max)
{
    typename std::map<U, T>::const_iterator i = map.cbegin();
    if (map.lastKey()<min)
        return i;

    else if (map.begin()->first>max)
        return i;

    else {
        while (i->first<min)
            ++i;

        typename std::map<U, T>::const_iterator biggest = i;
        for (; i != map.cend() && i->first<=max; ++i)
            if (*i > *biggest )  biggest = i;

        return biggest;
    }

}
/**
 * @brief We assume that min and max are in the values of the map
 * @param map
 * @param min
 * @param max
 * @return a pointer to the element with the smallest value
 */
template <class U, class T>
typename QMap<U, T>::const_iterator map_min(const QMap<U, T> &map)
{
    /* return std::min_element(map.begin(), map.end(),
                            [](const QMap<U, T>& p1, const QMap<U, T>& p2) {
                                return p1.values() < p2.values(); });
    */
    typename QMap<U, T>::const_iterator i = map.cbegin();

    typename QMap<U, T>::const_iterator smallest = i;
    for (; i != map.cend(); ++i)
        if (*i < *smallest )  smallest = i;

    return smallest;

}

/**
 * @brief We assume that min and max are in the values of the map
 * @param map
 * @param min
 * @param max
 * @return Returns a pointer to the element with the largest value
 */
template <class U, class T>
typename QMap<U, T>::const_iterator map_min(const QMap<U, T> &map, U min, U max)
{
    typename QMap<U, T>::const_iterator i = map.cbegin();
    if (map.lastKey()<min)
        return i;

    else if (map.firstKey()>max)
        return i;

    else {
        while (i.key()<min)
            ++i;

        typename QMap<U, T>::const_iterator smallest = i;
        for (; i != map.cend() && i.key()<=max; ++i)
            if (*i < *smallest )  smallest = i;

        return smallest;
    }

}
template <class U, class T>
T multimap_max_value(const QMultiMap<U, T> &map)
{
    typename QMultiMap<U, T>::const_iterator i = map.cbegin();
    T max = i.value();
    while (std::next(i) != map.cend()) {
        i++;
        max = std::max(max, i.value());
    }
    return max;
}

template <class U, class T>
T multimap_min_value(const QMultiMap<U, T> &map)
{
    typename QMultiMap<U, T>::const_iterator i = map.cbegin();
    T min = i.value();
    while (std::next(i) != map.cend()) {
        i++;
        min = std::min(min, i.value());
    }
    return min;
}
// --------------------------------
// can replace with std::accumulate(vector.begin(), vector.end(), T(0))
template<typename T>
T sum(const QList<T> &vector)
{
    T s = 0;
   /* std::for_each(vector.begin(), vector.end(), [&s](T& v){
        s += v;
    });*/
    for (const auto& v : vector) {
        s += v;
    }
    return s;
}

template<typename T>
T sum2(const QList<T> &vector)
{
    T sum = 0;
    /*std::for_each(vector.cbegin(), vector.cend(), [&sum](T& v){
        sum += v * v;
    });*/
    for (const auto &v : vector) {
        sum += v * v;
    }
    return sum;
}

template <template<typename...> class Container, class T >
T sumShifted(const Container<T> &vector, const T &shift)
{
    T sum = 0;
    /*std::for_each(vector.cbegin(), vector.cend(), [&sum, &shift](T& v){
        sum += v + shift;
    });*/
     for (const auto& v : vector) {
        sum += v + shift;
    }
    return sum;
}

template <template<typename...> class Container, class T >
T sum2Shifted(const Container<T> &vector, const T &shift)
{
    T sum = 0;
    /*std::for_each(vector.cbegin(), vector.cend(), [&sum, &shift](T& v){
        sum += (v + shift) * (v + shift);
    });*/
     for (const auto& v : vector)
        sum += (v + shift) * (v + shift);


    return sum;
}

template <template<typename...> class Container, class T >
T mean(const Container<T> &vector)
{
    return sum(vector)/ T(vector.size());
}

/**
 * @brief normalized sinc function, used with fftw
 * @param L the length of the gate
 */
template <typename T>
T sinc(const T x, const T L=1)
{
    if (x == T(0))
        return T(L);
    else
        return (sin( x * L) / x );
}

template<typename T >
QMap<T, T> normalize_map(const QMap<T, T> &map, const T max = 1)
{
    QMap<T, T> result;
    if (!map.empty()) {
        const T max_value = map_max(map).value();

        // can be done with std::generate !!
        for( auto it = map.begin(); it != map.end(); ++it)
            result[it.key()] = (it.value() / max_value)*max;

    }

    return result;
}

template <template<typename...> class Container, class T >
Container<T, T> normalize_map(const Container<T, T> &map, const T max = 1)
{
    Container<T, T> result;
    if (!map.empty()) {
        const T max_value = map_max(map)->second;

        // can be done with std::generate !!
        for( typename Container<T, T>::const_iterator it = map.begin(); it != map.end(); ++it)
            result[it->first] = (it->second / max_value)*max;

    }

    return result;
}

/**
 @brief This function transforms a Container (template class, ex:QList) turning its minimum value to "from" and its maximum value is "to" and adjusting other values accordingly
 **/
template <template<typename...> class Container, class T >
Container<T> stretch_vector(const Container<T> &vector, const T from, const T to)
{
    Container<T> histo;
    typename Container<T>::const_iterator it = vector.begin(); // here the template class is a container::const_iterator
    if (it != vector.end()) {
        const std::pair<typename Container<T>::const_iterator, typename Container<T>::const_iterator> min_max = std::minmax_element(vector.begin(), vector.end());
        const T min = *min_max.first;
        const T max = *min_max.second;

        if (min < max) {
            for (const auto& val : vector)
                histo.push_back(from + (to - from) * (val - min) / (max - min));

        } else // Just 1 value... set it to "from" (setting it to "to" could also be done though...)
            histo.push_back(to);

    }
    return histo;
}


/**
 * @brief Calcule la probabilité qu'une variable gaussienne soit dans l'intervalle [a, b].
 *
 * On considère une loi normale de moyenne \f$\mu\f$ et d'écart-type \f$\sigma\f$ :
 * \f[
 * f(x) = \frac{1}{\sigma \sqrt{2\pi}}
 *        \exp\!\left(-\frac{(x - \mu)^2}{2\sigma^2}\right).
 * \f]
 *
 * La probabilité que la variable aléatoire \f$X\f$ appartienne à [a, b] est donnée par :
 * \f[
 * P(a \leq X \leq b) =
 * \frac{1}{2}
 * \left[
 * \operatorname{erf}\!\left(\frac{b - \mu}{\sigma \sqrt{2}}\right)
 * - \operatorname{erf}\!\left(\frac{a - \mu}{\sigma \sqrt{2}}\right)
 * \right].
 * \f]
 *
 * @param a borne inférieure de l'intervalle
 * @param b borne supérieure de l'intervalle
 * @param mu moyenne de la loi normale (par défaut 0.0)
 * @param sigma écart-type (>0) de la loi normale (par défaut 1.0)
 * @return Probabilité que X soit dans [a, b].
 *
 * @throw std::invalid_argument si sigma <= 0
 */
inline double diff_erf(double a, double b,
                       double mu = 0.0,
                       double sigma = 1.0)
{
#if DEBUG
    if (sigma <= 0.0) {
        throw std::invalid_argument("sigma must be > 0");
    }
#endif
    const double z_a = (a - mu) / (sigma * M_SQRT2);
    const double z_b = (b - mu) / (sigma * M_SQRT2);

    return 0.5 * (std::erf(z_b) - std::erf(z_a));
}

/**
 * @brief Calcule le z-score associé à un intervalle de confiance bilatéral (version noexcept).
 *
 * Pour un niveau de risque \f$\alpha\f$, le z-score est :
 * \f[
 * z_{\alpha/2} = \Phi^{-1}(1 - \alpha/2)
 * \f]
 * où \f$\Phi^{-1}\f$ est la fonction quantile de la loi normale standard.
 *
 * Exemples :
 * - alpha = 0.05  →  z ≈ 1.96  (IC à 95%)
 * - alpha = 0.10  →  z ≈ 1.645 (IC à 90%)
 * - alpha = 0.01  →  z ≈ 2.576 (IC à 99%)
 *
 * @param alpha Niveau de risque (0 < alpha < 1).
 * @return z-score \f$z_{\alpha/2}\f$, ou NaN si alpha est invalide.
 *  * https://en.wikipedia.org/wiki/Standard_score
 */
[[nodiscard]] inline double zScore(double alpha) noexcept
{
    if (alpha <= 0.0 || alpha >= 1.0) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    // Convertit alpha en probabilité cumulée
    const double p = 1.0 - alpha / 2.0;

    // Coefficients de l’approximation rationnelle (Peter J. Acklam, 2003)
    static const double a1 = -3.969683028665376e+01;
    static const double a2 =  2.209460984245205e+02;
    static const double a3 = -2.759285104469687e+02;
    static const double a4 =  1.383577518672690e+02;
    static const double a5 = -3.066479806614716e+01;
    static const double a6 =  2.506628277459239e+00;

    static const double b1 = -5.447609879822406e+01;
    static const double b2 =  1.615858368580409e+02;
    static const double b3 = -1.556989798598866e+02;
    static const double b4 =  6.680131188771972e+01;
    static const double b5 = -1.328068155288572e+01;

    static const double c1 = -7.784894002430293e-03;
    static const double c2 = -3.223964580411365e-01;
    static const double c3 = -2.400758277161838e+00;
    static const double c4 = -2.549732539343734e+00;
    static const double c5 =  4.374664141464968e+00;
    static const double c6 =  2.938163982698783e+00;

    static const double d1 =  7.784695709041462e-03;
    static const double d2 =  3.224671290700398e-01;
    static const double d3 =  2.445134137142996e+00;
    static const double d4 =  3.754408661907416e+00;

    const double plow  = 0.02425;
    const double phigh = 1 - plow;

    double q, r;
    if (p < plow) {
        q = std::sqrt(-2 * std::log(p));
        return (((((c1*q+c2)*q+c3)*q+c4)*q+c5)*q+c6) /
               ((((d1*q+d2)*q+d3)*q+d4)*q+1);
    } else if (p <= phigh) {
        q = p - 0.5;
        r = q * q;
        return (((((a1*r+a2)*r+a3)*r+a4)*r+a5)*r+a6)*q /
               (((((b1*r+b2)*r+b3)*r+b4)*r+b5)*r+1);
    } else {
        q = std::sqrt(-2 * std::log(1 - p));
        return -(((((c1*q+c2)*q+c3)*q+c4)*q+c5)*q+c6) /
               ((((d1*q+d2)*q+d3)*q+d4)*q+1);
    }
}


/**
 * @brief N compute Normal law = Gauss law // see dnorm()
 * @param x
 * @param mean
 * @param stddev
 * @return
 */
template<typename T>
inline T N(T x, T mean = 0, T stddev = 1) {
    return exp(-pow(x - mean, 2)/(2*pow(stddev, 2)))/(stddev*M_SQRT2PI);
}

// Function used for activity curve
std::vector<double> binomialeCurveByLog(const int n, const double alpha= .05, const int q_frac = 500);

std::vector<double> inverseCurve(const std::vector<double> &Rp, const int x_frac = 500);

double findOnOppositeCurve (const double x, const std::vector<double> &Gx);

template <template<typename...> class Container, class T >
bool container_contains(const Container<T>& vec, const T& value) {
    return !std::none_of(vec.begin(), vec.end(), [&value](const T& element) {
        return element == value;
    });
}

class Chronometer
{
private:
    std::string _comment;
    std::chrono::time_point<std::chrono::high_resolution_clock> _start;

public:
    Chronometer();
    explicit Chronometer(std::string comment);


    virtual ~Chronometer();

    void display();
    std::chrono::microseconds eval();
};

// Argsort : retourne les indices qui trieraient le vecteur `v` dans l'ordre croissant
template <typename T>
std::vector<size_t> argsort(const std::vector<T>& v) {
    std::vector<size_t> indices(v.size());
    for (size_t i = 0; i < v.size(); ++i)
        indices[i] = i;

    std::sort(indices.begin(), indices.end(),
              [&v](size_t i1, size_t i2) { return v[i1] < v[i2]; });

    return indices;
}

double erfcInv_approx(double x);
double normInv(double p);

#pragma mark UCV
/* -----------------------------------------------------------------
   1.  Fonction auxiliaire : noyau gaussien normalisé
   ----------------------------------------------------------------- */
inline double gaussian_kernel(double u) noexcept
{
    static constexpr double INV_SQRT_2PI = 0.3989422804014327; // 1/√(2π)
    return INV_SQRT_2PI * std::exp(-0.5 * u * u);
}
double ucv_criterion(double h,
                     const std::vector<double>& x,
                     const std::size_t n);

double golden_minimize(const std::vector<double>& x,
                          double a, double b,
                          double tol = 1e-8,
                          int max_iter = 100);

double brent_minimize(const std::vector<double>& x,
                      double a, double b,
                      double tol = 1e-8,
                      int max_iter = 100);


double bw_ucv(const std::vector<double>& data);

double bw_ucv_naive(const std::vector<double>& data);

#pragma mark UCV with FFTW

/* Retourne la plus petite puissance de deux ≥ n */
static std::size_t next_pow2(std::size_t n)
{
    std::size_t p = 1;
    while (p < n) p <<= 1;
    return p;
}
/* -----------------------------------------------------------------
   2.  Construction de l’histogramme (empirical density)
   ----------------------------------------------------------------- */
struct Histogram
{
    std::vector<double> bins;   // nombre d’observations dans chaque bin
    double               delta; // pas de la grille
    double               xmin;  // origine (bord gauche du premier bin)
    explicit Histogram(const std::vector<double>& data,
                       std::size_t nbins = 0)
    {
        if (data.empty())
            throw std::invalid_argument("Histogram: empty data");
        const double data_min = *std::min_element(data.begin(), data.end());
        const double data_max = *std::max_element(data.begin(), data.end());
        const double range = data_max - data_min;

        // Si toutes les valeurs sont identiques, on crée un petit intervalle artificiel
        const double eps = std::numeric_limits<double>::epsilon();
        const double effective_range = (range < eps) ? eps : range;
        // Choix du pas de grille : on veut au moins 256 bins, mais pas plus de 2^14
        const std::size_t min_bins = 256;
        const std::size_t max_bins = 16384;
        if (nbins == 0) {
            nbins = static_cast<std::size_t>(std::ceil(effective_range));
            nbins = std::max(min_bins, std::min(max_bins, nbins));
        }
        delta = effective_range / static_cast<double>(nbins);
        xmin  = data_min - delta * 0.5;          // centre du premier bin = xmin + delta/2
        bins.assign(nbins, 0.0);
        for (double v : data) {
            std::size_t idx = static_cast<std::size_t>(std::floor((v - xmin) / delta));
            // protection contre les débordements numériques
            if (idx >= nbins) idx = nbins - 1;
            bins[idx] += 1.0;
        }
    }
};

/* -----------------------------------------------------------------
   3.  Calcul du critère UCV à l’aide de la FFT
   ----------------------------------------------------------------- */
class UCV_FFT
{
public:
    explicit UCV_FFT(const std::vector<double>& data)
        : n_(data.size()),
        hist_(data),
        N_(next_pow2(2 * hist_.bins.size()))
    {
        const double delta = hist_.delta;

        // ---------- Allocation ----------
        in_   = (double*)fftw_malloc(sizeof(double) * N_);
        out_  = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * (N_/2 + 1));
        tmp_  = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * (N_/2 + 1));
        prod_ = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * (N_/2 + 1));

        // ---------- Histogramme NORMALISÉ (clé !) ----------
        std::fill(in_, in_ + N_, 0.0);
        for (std::size_t i = 0; i < hist_.bins.size(); ++i) {
            in_[i] = hist_.bins[i] / delta;   // 🔥 correction majeure
        }

        // ---------- FFT histogramme ----------
        plan_hist_ = fftw_plan_dft_r2c_1d((int)N_, in_, out_, FFTW_ESTIMATE);
        fftw_execute(plan_hist_);

        // ---------- constante correcte ----------
        const_factor_ = 1.0 / (2.0 * n_ * std::sqrt(M_PI));
    }

    ~UCV_FFT()
    {
        fftw_destroy_plan(plan_hist_);
        fftw_destroy_plan(plan_kernel_);
        fftw_destroy_plan(plan_ifft_);
        fftw_free(in_);
        fftw_free(out_);
        fftw_free(tmp_);
        fftw_free(prod_);
    }

    double operator()(double h) const
    {
        if (h <= 0.0)
            return std::numeric_limits<double>::infinity();

        const double inv_h = 1.0 / h;
        const double delta = hist_.delta;
        const std::size_t nbins = hist_.bins.size();

        // ---------- noyau discret ----------
        std::fill(in_, in_ + N_, 0.0);

        for (std::size_t k = 0; k < nbins; ++k) {
            double u = (double)k * delta * inv_h;
            in_[k] = (1.0 / h) * gaussian_kernel(u);
        }

        for (std::size_t k = 1; k < nbins; ++k) {
            in_[N_ - k] = in_[k];
        }

        // ---------- FFT noyau ----------
        plan_kernel_ = fftw_plan_dft_r2c_1d((int)N_, in_, tmp_, FFTW_ESTIMATE);
        fftw_execute(plan_kernel_);

        // ---------- produit fréquentiel ----------
        for (std::size_t i = 0; i < N_/2 + 1; ++i) {
            const double a = out_[i][0];
            const double b = out_[i][1];
            const double c = tmp_[i][0];
            const double d = tmp_[i][1];

            prod_[i][0] = a*c - b*d;
            prod_[i][1] = a*d + b*c;
        }

        // ---------- IFFT ----------
        plan_ifft_ = fftw_plan_dft_c2r_1d((int)N_, prod_, in_, FFTW_ESTIMATE);
        fftw_execute(plan_ifft_);

        const double scale = 1.0 / (double)N_;
        for (std::size_t i = 0; i < N_; ++i)
            in_[i] *= scale;

        // ---------- somme ----------
        double sum_all = 0.0;
        for (std::size_t k = 0; k < nbins; ++k) {
            sum_all += in_[k] * delta * delta;   // 🔥 correct
        }

        // ---------- retrait diagonale ----------
        const double K0 = 1.0 / (h * std::sqrt(2.0 * M_PI));
        const double sum_ij = sum_all - n_ * K0;

        // ---------- UCV ----------
        const double term1 = sum_ij / (n_ * (n_ - 1));
        const double uc = term1 - const_factor_ / h;

        return uc;
    }

private:
    std::size_t n_;
    Histogram hist_;
    std::size_t N_;

    double const_factor_;

    double* in_;
    fftw_complex* out_;
    fftw_complex* tmp_;
    fftw_complex* prod_;

    mutable fftw_plan plan_hist_;
    mutable fftw_plan plan_kernel_;
    mutable fftw_plan plan_ifft_;
};

/* -----------------------------------------------------------------
   4.  Recherche du minimum (Brent) – même principe que la version
       O(n²) mais le critère est maintenant calculé via FFT.
   ----------------------------------------------------------------- */

double brent_minimize(const UCV_FFT& ucv,
                      double a, double b,
                      double tol = 1e-8,
                      int max_iter = 100);

double bw_ucv_fft(const std::vector<double>& data);


static inline double phi(double z) {
    static const double inv_sqrt_2pi = 0.39894228040143267794;
    return inv_sqrt_2pi * std::exp(-0.5 * z * z);
};

double ucv_score_gaussian(std::vector<double> x, double h);
double bw_ucv_gaussian(std::vector<double> x,
                       double hmin,
                       double hmax,
                       int grid = 200);

#pragma mark SJ

// ================================================================
//  Utilitaires
// ================================================================

static double mean_vec(const std::vector<double>& x) {
    return std::accumulate(x.begin(), x.end(), 0.0) / x.size();
}

static double var_vec(const std::vector<double>& x) {
    double m = mean_vec(x);
    double s = 0.0;
    for (double v : x) s += (v - m) * (v - m);
    return s / (x.size() - 1);
}

// ================================================================
//  Noyaux dérivés (convolutions gaussiennes)
//  Reproduit exactement les formules de Sheather & Jones (1991)
// ================================================================

// phi^(r)(x) = r-ième dérivée de la densité normale standard
// phi^(4)(x) = (x^4 - 6x^2 + 3) * phi(x)
// phi^(6)(x) = (x^6 - 15x^4 + 45x^2 - 15) * phi(x)

static double phi4(double x) {
    double x2 = x * x;
    return (x2*x2 - 6.0*x2 + 3.0) * std::exp(-0.5*x2) / std::sqrt(2.0*M_PI);
}

static double phi6(double x) {
    double x2 = x * x;
    return (x2*x2*x2 - 15.0*x2*x2 + 45.0*x2 - 15.0)
           * std::exp(-0.5*x2) / std::sqrt(2.0*M_PI);
}

// ================================================================
//  Estimateurs des fonctionnelles de densité
//  S_r(h) = (1/n²) Σ_i Σ_j phi^(r)((x_i - x_j)/h) / h^(r+1)
// ================================================================
// fonction trop lente, préférer la version fft
static double S4(const std::vector<double>& x, double h) {
    int n = x.size();
    double sum = 0.0;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            sum += phi4((x[i] - x[j]) / h);
    return sum / (static_cast<double>(n) * static_cast<double>(n) * std::pow(h, 5));
}

inline double S6(const std::vector<double>& x, double h) {
    int n = x.size();
    double sum = 0.0;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            sum += phi6((x[i] - x[j]) / h);
    return sum / (static_cast<double>(n) * static_cast<double>(n) * std::pow(h, 7));
}

// Règle pratique : M = première puissance de 2 >= max(1024, n/100)
static int chooseFFtSize(int n) {
    int M = 1024;
    while (M < n / 100) M <<= 1;  // doublement jusqu'à n/100
    return M;
}

/**
 * @brief Estimates θ₄₄ = ∫ f⁽⁴⁾(x)² dx via FFT — O(n log n)
 *
 * @details
 * Exploits the convolution theorem:
 * @f[
 *   \hat\theta_{44} = \frac{1}{n^2} \sum_i \sum_j \phi^{(4)}\!\left(\frac{x_i-x_j}{h}\right) h^{-5}
 *                   = \frac{1}{n} \int \hat f_h^{(4)}(x)^2 \, dx
 * @f]
 * In the Fourier domain, with @f$ \hat\phi(\omega) = e^{-\omega^2/2} @f$:
 * @f[
 *   \hat\theta_{44} = \frac{1}{2\pi n^2}
 *     \int_{-\infty}^{+\infty} \omega^4 \, e^{-\omega^2 h^2} \,
 *     \left|\hat p(\omega)\right|^2 d\omega
 * @f]
 * where @f$ \hat p @f$ is the DFT of the binned data.
 *
 * @param x  Input sample.
 * @param h  Pilot bandwidth.
 * @param M  FFT grid size (default 1024, must be a power of 2).
 * @return   Estimate of @f$ \hat\theta_{44} @f$.
 */
static double S4_fft(const std::vector<double>& x, double h, int M = 1024)
{
    const int    n   = static_cast<int>(x.size());
    const double xmin = *std::min_element(x.begin(), x.end());
    const double xmax = *std::max_element(x.begin(), x.end());

    // ----------------------------------------------------------------
    // 1. Grille régulière avec padding 4h de chaque côté
    // ----------------------------------------------------------------
    const double a     = xmin - 4.0 * h;
    const double b     = xmax + 4.0 * h;
    const double delta = (b - a) / static_cast<double>(M);   // pas de grille

    // ----------------------------------------------------------------
    // 2. Binning linéaire (linear binning) — O(n)
    //    Chaque observation est répartie entre les deux bins voisins
    //    proportionnellement à sa distance aux centres de bins.
    // ----------------------------------------------------------------
    std::unique_ptr<double[], decltype(&fftw_free)>
        grid(static_cast<double*>(fftw_malloc(M * sizeof(double))), fftw_free);

    std::fill(grid.get(), grid.get() + M, 0.0);

    for (int i = 0; i < n; ++i) {
        const double z   = (x[i] - a) / delta;   // position en unités de bins
        const int    k   = static_cast<int>(std::floor(z));
        const double frac = z - static_cast<double>(k);

        if (k >= 0 && k < M - 1) {
            grid[k]     += (1.0 - frac) / static_cast<double>(n);
            grid[k + 1] +=        frac  / static_cast<double>(n);
        } else if (k == M - 1) {
            grid[k]     += (1.0 - frac) / static_cast<double>(n);
        }
    }

    // ----------------------------------------------------------------
    // 3. FFT forward : grid → spectrum
    // ----------------------------------------------------------------
    const int complexSize = M / 2 + 1;

    std::unique_ptr<double[], decltype(&fftw_free)>
        spectrum(static_cast<double*>(
                     fftw_malloc(2 * complexSize * sizeof(double))), fftw_free);

    fftw_plan plan_fwd = fftw_plan_dft_r2c_1d(
        M, grid.get(),
        reinterpret_cast<fftw_complex*>(spectrum.get()),
        FFTW_ESTIMATE);

    fftw_execute(plan_fwd);
    fftw_destroy_plan(plan_fwd);

    // ----------------------------------------------------------------
    // 4. Intégration dans le domaine fréquentiel
    //
    //   θ₄₄ = (1 / 2π) × Σₖ ωₖ⁴ × exp(-ωₖ² h²) × |p̂(ωₖ)|² × Δω
    //
    //   avec ωₖ = 2πk / (M × delta)   (fréquence angulaire du bin k)
    //   et   Δω = 2π / (M × delta)
    //
    //   Le facteur 1/fftLen² vient de la normalisation FFTW (non normalisée).
    // ----------------------------------------------------------------
    double sum = 0.0;
    const double L    = static_cast<double>(M) * delta;  // longueur du domaine
    const double dOmega = 2.0 * M_PI / L;                // pas fréquentiel
    const double norm2  = static_cast<double>(M) * static_cast<double>(M); // correction FFTW

    for (int k = 0; k < complexSize; ++k) {
        const double omega  = static_cast<double>(k) * dOmega;
        const double omega2 = omega * omega;
        const double omega4 = omega2 * omega2;

        // Module² de p̂(ωₖ), corrigé de la normalisation FFTW
        const double re  = spectrum[2 * k]     / static_cast<double>(M);
        const double im  = spectrum[2 * k + 1] / static_cast<double>(M);
        const double mod2 = re * re + im * im;

        // Filtre gaussien d'ordre 4
        const double gauss = std::exp(-omega2 * h * h);

        // Facteur 2 pour les fréquences négatives (sauf k=0 et k=M/2)
        const double weight = (k == 0 || k == M / 2) ? 1.0 : 2.0;

        sum += weight * omega4 * gauss * mod2;
    }

    // Normalisation finale : Δω / (2π)
    return sum * dOmega / (2.0 * M_PI);
}


/**
 * @brief Estimates θ₂₄ = ∫ f⁽⁶⁾(x)² dx via FFT — O(n log n)
 *
 * @details Identical to S4_fft() but uses @f$ \omega^6 @f$ in the integrand:
 * @f[
 *   \hat\theta_{24} = \frac{1}{2\pi n^2}
 *     \int \omega^6 \, e^{-\omega^2 h^2} \left|\hat p(\omega)\right|^2 d\omega
 * @f]
 *
 * @param x  Input sample.
 * @param h  Pilot bandwidth (g₂).
 * @param M  FFT grid size (default 1024, must be a power of 2).
 * @return   Estimate of @f$ \hat\theta_{24} @f$.
 */
static double S6_fft(const std::vector<double>& x, double h, int M = 1024)
{
    const int    n    = static_cast<int>(x.size());
    const double xmin = *std::min_element(x.begin(), x.end());
    const double xmax = *std::max_element(x.begin(), x.end());

    const double a     = xmin - 4.0 * h;
    const double b     = xmax + 4.0 * h;
    const double delta = (b - a) / static_cast<double>(M);

    std::unique_ptr<double[], decltype(&fftw_free)>
        grid(static_cast<double*>(fftw_malloc(M * sizeof(double))), fftw_free);

    std::fill(grid.get(), grid.get() + M, 0.0);

    for (int i = 0; i < n; ++i) {
        const double z    = (x[i] - a) / delta;
        const int    k    = static_cast<int>(std::floor(z));
        const double frac = z - static_cast<double>(k);

        if (k >= 0 && k < M - 1) {
            grid[k]     += (1.0 - frac) / static_cast<double>(n);
            grid[k + 1] +=        frac  / static_cast<double>(n);
        } else if (k == M - 1) {
            grid[k]     += (1.0 - frac) / static_cast<double>(n);
        }
    }

    const int complexSize = M / 2 + 1;

    std::unique_ptr<double[], decltype(&fftw_free)>
        spectrum(static_cast<double*>(
                     fftw_malloc(2 * complexSize * sizeof(double))), fftw_free);

    fftw_plan plan_fwd = fftw_plan_dft_r2c_1d(
        M, grid.get(),
        reinterpret_cast<fftw_complex*>(spectrum.get()),
        FFTW_ESTIMATE);

    fftw_execute(plan_fwd);
    fftw_destroy_plan(plan_fwd);

    double sum = 0.0;
    const double L       = static_cast<double>(M) * delta;
    const double dOmega  = 2.0 * M_PI / L;

    for (int k = 0; k < complexSize; ++k) {
        const double omega  = static_cast<double>(k) * dOmega;
        const double omega2 = omega * omega;
        const double omega6 = omega2 * omega2 * omega2;   // ← ω⁶ ici

        const double re   = spectrum[2 * k]     / static_cast<double>(M);
        const double im   = spectrum[2 * k + 1] / static_cast<double>(M);
        const double mod2 = re * re + im * im;

        const double gauss  = std::exp(-omega2 * h * h);
        const double weight = (k == 0 || k == M / 2) ? 1.0 : 2.0;

        sum += weight * omega6 * gauss * mod2;
    }

    return sum * dOmega / (2.0 * M_PI);
}

// ================================================================
//  Estimateurs de référence gaussiens (plug-in initial)
//  Formules de Jones & Sheather (1991), eq. (15)-(16)
// ================================================================
/*
// theta_22 pour une gaussienne de variance sigma²
static double theta22_gauss(double sigma, int n) {
    return 1.0 / (4.0 * std::sqrt(M_PI) * std::pow(sigma, 5) * n);
}

// theta_24 pour une gaussienne de variance sigma²
static double theta24_gauss(double sigma, int n) {
    return -3.0 / (8.0 * std::sqrt(M_PI) * std::pow(sigma, 7) * n);
}

// theta_44 pour une gaussienne de variance sigma²
static double theta44_gauss(double sigma, int n) {
    return 3.0 / (16.0 * std::sqrt(M_PI) * std::pow(sigma, 9) * n);
}
*/
// ================================================================
//  Bandwidths pilotes a et b (étape 1 de SJ)
//  Reproduit R : scale = min(sd, IQR/1.349)
// ================================================================

static double iqr_vec(std::vector<double> x) {
    std::sort(x.begin(), x.end());
    int n = x.size();
    // interpolation linéaire comme R
    auto quantile = [&](double p) {
        double h = (n - 1) * p;
        int lo = (int)h;
        int hi = lo + 1;
        if (hi >= n) return x[n-1];
        return x[lo] + (h - lo) * (x[hi] - x[lo]);
    };
    return quantile(0.75) - quantile(0.25);
}

inline double scale_factor(const std::vector<double>& x) {
    const double s  = std::sqrt(var_vec(x));
    const double iq = iqr_vec(x) / 1.349;
    // Si IQR nul, on replie sur l'écart-type seul
    return (iq > 0.0) ? std::min(s, iq) : s;
}

// ================================================================
//  Méthode SJ — Direct Plug-In (DPI)
//  Reproduit bw.SJ(x, method="dpi") de R
// ================================================================

double bw_SJ_dpi(const std::vector<double>& x);
// ================================================================
//  Méthode SJ — Solve-The-Equation (STE)
//  Reproduit bw.SJ(x, method="ste") de R  — plus précis que DPI
// ================================================================

// Équation à résoudre : h - (c1 / S4(x, alpha2 * h^(5/7)))^(1/5) = 0
/**
 * @brief Fixed-point equation for the Sheather-Jones STE bandwidth selector.
 *
 * @details Evaluates:
 * @f[
 *   f(h) = h - \left(\frac{c_1}{\hat\theta_{44}(\alpha_2 \cdot h^{5/7})}\right)^{1/5}
 * @f]
 * The STE bandwidth is the root of this equation, found by bisection in bw_SJ_ste().
 *
 * Returns NaN if the functional estimate is non-positive (degenerate case),
 * which bisect() must handle explicitly.
 *
 * @param h       Current bandwidth candidate.
 * @param x       Input data sample.
 * @param alpha2  Intermediate scale factor (1.357 × |θ₂₄/θ₄₄|^(1/7)).
 * @param c1      Normalisation constant (1 / (2√π × n)).
 * @param M       FFT grid size for S4_fft (must be a power of 2).
 *
 * @return f(h) = h - (c1/θ̂₄₄)^(1/5), or NaN if θ̂₄₄ ≤ 0.
 *
 * @see bw_SJ_ste(), S4_fft(), bisect()
 */
inline double sj_equation(double h,
                          const std::vector<double>& x,
                          double alpha2,
                          double c1,
                          int    M = 1024)
{
    const double h_pilot = alpha2 * std::pow(h, 5.0 / 7.0);
    if (h_pilot <= 0.0)
        return std::numeric_limits<double>::quiet_NaN();

    const double denom    = S4_fft(x, h_pilot, M);
    const double absDenom = std::abs(denom);

    if (absDenom < 1e-15)
        return std::numeric_limits<double>::quiet_NaN();

    return h - std::pow(c1 / absDenom, 0.2);
}

// Bisection pour résoudre l'équation STE (comme uniroot dans R)
template <typename F>
double bisect(F f, double a, double b, double tol, int max_iter)
{
    double fa = f(a);
    double fb = f(b);

    if (std::isnan(fa) || std::isnan(fb) || fa * fb > 0.0)
        return std::numeric_limits<double>::quiet_NaN();

    for (int i = 0; i < max_iter; ++i) {
        const double m  = 0.5 * (a + b);
        const double fm = f(m);

        // Point dégénéré : S4_fft a échoué, on resserre prudemment
        if (std::isnan(fm)) {
            if (fa > 0.0) b = m;
            else          a = m;
            continue;
        }

        if (std::abs(b - a) < tol || std::abs(fm) < tol * 1e-3)
            return m;

        if (fa * fm < 0.0) { b = m; fb = fm; }
        else               { a = m; fa = fm; }
    }

    return 0.5 * (a + b);
}

/**
 * @brief Estimates the optimal KDE bandwidth using the Sheather-Jones
 *        Solve-The-Equation (STE) plug-in method.
 *
 * @details
 * Implements the Sheather-Jones STE selector as described in:
 *
 * > Sheather, S.J. & Jones, M.C. (1991).
 * > *A reliable data-based bandwidth selection method for kernel density estimation.*
 * > Journal of the Royal Statistical Society, Series B, **53**(3), 683–690.
 *
 * The algorithm proceeds in three stages:
 *
 * **1. Pilot bandwidths (plug-in stage)**
 * Two pilot bandwidths are computed from the data scale factor σ:
 * @f[
 *   g_1 = 1.24 \, \sigma \, n^{-1/7}, \qquad
 *   g_2 = 1.23 \, \sigma \, n^{-1/9}
 * @f]
 * These are used to estimate the functionals:
 * @f[
 *   \hat\theta_{44} = S_4(x, g_1), \qquad
 *   \hat\theta_{24} = S_6(x, g_2)
 * @f]
 * where @f$ S_r @f$ denotes the empirical estimate of the @f$ r @f$-th
 * derivative functional of the density, computed via @f$ \phi^{(r)} @f$.
 *
 * **2. Intermediate bandwidth α₂**
 * @f[
 *   \alpha_2 = 1.357
 *     \left|\frac{\hat\theta_{24}}{\hat\theta_{44}}\right|^{1/7}
 * @f]
 *
 * **3. Fixed-point equation (STE)**
 * The final bandwidth @f$ h @f$ is the solution of:
 * @f[
 *   h = \left(\frac{c_1}{S_4(x,\, \alpha_2 \cdot h^{5/7})}\right)^{1/5},
 *   \qquad c_1 = \frac{1}{2\sqrt{\pi}\, n}
 * @f]
 * solved by bisection over an interval automatically widened if necessary.
 *
 * **Fallback to Silverman's rule of thumb**
 * @f[
 *   h_{\text{Silverman}} = 0.9 \, \sigma \, n^{-1/5}
 * @f]
 * is returned when:
 * - @f$ |\hat\theta_{44}| @f$ or @f$ |\hat\theta_{24}| @f$ is numerically zero,
 * - no sign change is found in the bisection interval after 20 enlargement steps,
 * - @c bisect() returns NaN.
 *
 * @note Complexity is dominated by @f$ S_4 @f$ and @f$ S_6 @f$,
 *       each @f$ O(n^2) @f$. For large traces (@f$ n > 10^4 @f$),
 *       consider a FFT-based implementation.
 *
 * @param x        Input data sample (run-part of the MCMC trace).
 *                 Must contain at least 2 distinct values.
 * @param tol      Convergence tolerance for the bisection solver.
 *                 Defaults to @c 1e-6.
 * @param max_iter Maximum number of bisection iterations.
 *                 Defaults to @c 100.
 *
 * @return Optimal bandwidth @f$ h > 0 @f$ for Gaussian kernel density estimation,
 *         or Silverman's fallback if the STE equation cannot be solved.
 *
 * @see S4(), S6(), bisect(), sj_equation(), scale_factor()
 * @see Sheather & Jones (1991), JRSS-B 53(3):683–690.
 */
double bw_SJ_ste(const std::vector<double>& x,
                 double tol      = 1e-6,
                 int    max_iter = 100);

#pragma mark tempering

// a) Exponentiel décroit (α > 1)
inline double schedule_exp_pow(double sigma, double T, double alpha = 2.0)
{
    return sigma * std::pow(alpha, T);          // sigma_T = sigma * α^T
}
// b) Exponentiel doux (k > 0)
inline double schedule_exp(double sigma, double T, double k = 0.7)
{
    return sigma * std::exp(k * T);             // sigma_T = sigma * e^{kT}
}
// c) Linéaire (a > 0, b > 0)
inline double schedule_linear(double sigma, double T,
                              double a = 0.1, double b = 0.2)
{
    return sigma * (a + b * T);                 // sigma_T = sigma * (a + b·T)
}
// d) Inverse polynomial (c > 0)
inline double schedule_inverse_poly(double sigma, double T,
                                    double T_max, double c = 0.5)
{
    double factor = 1.0 / (1.0 + c * (T_max - T));
    return sigma * factor;                      // sigma_T = sigma / (1 + c·ΔT)
}
// e) Puissance (p > 0)
inline double schedule_power(double sigma, double T,
                             double T_max, double p = 0.5)
{
    double ratio = T / T_max;                   // entre 0 et 1
    return sigma * std::pow(ratio, p);          // sigma_T = sigma * (T/T_max)^p
}



#endif
