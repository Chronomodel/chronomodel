/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2023

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

#include <RefCurve.h>

#include <chrono>
#include <valarray>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>
#include <math.h>
#include <iterator>

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

const double gammaActivity[] = {1.,-1.,0.00501257,0.0589032,0.140868,0.222072,0.294314,0.356635,0.410058,0.455966,0.495647,0.530184,0.560456,0.587174,
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
double interpolate_value_from_curve(const double t, const QList<double> &curve, const double curveTmin, const double curveTmax);
double interpolate_value_from_curve(const double x, const std::vector<double> &curve, const double Xmin, const double Xmax);

double map_area(const QMap<double, double> &map);
float map_area(const QMap<float, float> &map);
double map_area(const QMap<int, double> &density);
double map_area(const std::map<double, double> &map);

inline double surface_on_theta (std::map<double, double>::const_iterator iter_on_theta );
const std::map<double, double> create_HPD2(const QMap<double, double> &density, const double threshold = 95.);
const std::map<double, double> create_HPD_mapping(const QMap<double, double> &density, std::map<double, double> &area_mapping, const double threshold = 95.);

const std::map<double, double> create_HPD_by_dichotomy(const QMap<double, double> &density, QList<QPair<double, QPair<double, double> > > &intervals_hpd, const double threshold);
const std::map<double, double> create_HPD_by_dichotomy(const std::map<double, double> &density, QList<QPair<double, QPair<double, double> > > &intervals_hpd, const double threshold);

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
            const decltype(vector.size()) idxMid = idxInf + decltype(vector.size()) (floor((idxSup - idxInf) / 2.));
            const T valueMid = vector.at(idxMid);

            if (value < valueMid)
                idxSup = idxMid;
            else
                idxInf = idxMid;

        } while (idxSup - idxInf > 1);

        T valueInf = vector.at(idxInf);
        T valueSup = vector.at(idxSup);

        // test si on a atteind la valeur
        if (std::abs(valueInf - value) <= 10E-300) {
            valueSup = value;
            idxSup = idxInf;

        } else  if (std::abs(valueSup - value) <= 10E-300) {
            valueInf = value;
            idxInf = valueSup;
        }

        T idx;
        if (std::abs(valueSup - valueInf) <= 10E-300) { // on recherche la taille du plateau, on élargie pour determiner le centre
            while (idxInf>0 && std::abs(valueInf - vector.at(idxInf-1)) <= 10E-300) {
                idxInf -= 1;
                valueInf = vector.at(idxInf);
            };
            while (idxSup<vector.size()-1 && std::abs(valueSup- vector.at(idxSup+1)) <= 10E-300) {
                idxSup += 1;
                valueSup = vector.at(idxSup);
            };

            idx = (idxSup+idxInf)/2.;

        } else {
            // On ressert l'intervale, une des bornes est sur le plateau
            while (idxInf>0 && std::abs(valueInf - vector.at(idxInf+1)) <= 10E-300) {
                idxInf += 1;
                valueInf = vector.at(idxInf);
            };
            while (idxSup<vector.size()-1 && std::abs(valueSup - vector.at(idxSup-1)) <= 10E-300) {
                idxSup -= 1;
                valueSup = vector.at(idxSup);
            };


            T prop = 0.;
            // prevent valueSup=valueInf because in this case prop = NaN
            if (valueSup > valueInf)
                prop = (value - valueInf) / (valueSup - valueInf);

            idx =  prop * (idxSup-idxInf);
            idx += T (idxInf);
            //auto idx2 = interpolate(value, valueInf, valueSup, T(idxInf), T (idxSup));
        }
        return idx;

    } else if (idxSup == idxInf) {
        return T (idxInf);
    }

    return T (0);
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

#endif
