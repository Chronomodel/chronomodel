/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

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

#include <vector>
#include <map>
#include <random>
#include <cmath>
#include <algorithm>
#include <math.h>
#include <iterator>

#include <QMap>
#include <QVector>
#include <QList>
#include <QDebug>

#ifdef DEBUG
#include <iostream>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846 //usefull to Windows
#endif

#ifndef M_PIl
#define M_PIl 3.14159265358979323846264338327950288419716939937510L //usefull in long double
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

double safeExp(const double& x, int n = 10);
double safeLog(const double& x, int n = 5);

void checkFloatingPointException(const QString& infos = QString());

template <typename T, typename V>
V interpolate(const T& x, const T& x1, const T& x2, const V& y1, const V& y2)
{
    Q_ASSERT(x1!=x2);
    return (y1 + (y2 - y1) * V((x - x1) / (x2 - x1)) );
}

template <typename T, typename U>
T interpolateValueInQMap(const U& key, const QMap<U, T>& map)
{
    if (key <= map.firstKey()) {
        return map.first();

    } else if (key >= map.lastKey()) {
        return map.last();

    } else {
        auto lIter =  map.lowerBound(key);
        auto uIter = lIter;
        uIter++;
        return interpolate(key, lIter.key(), uIter.key(), lIter.value(), uIter.value());
    }

 /*
    typename QMap<U, T>::iterator UIter = map.upperBound(key);
    T valueUpper = UIter.value();
    T keyUpper = UIter.key();
    typename QMap<U, T>::iterator lIter (--UIter);

    if (UIter == map.begin()) {
        return map.first();
    } else if (UIter)


    if (key <= keyUnder)
        return valueUnder;

    else if (lIter != map.end()) {
        typename QMap<U, T>::iterator UIter (++lIter);
        if (UIter != map.end()) {
            T valueUpper = lIter.value();
            T keyUpper = lIter.key();
            return interpolate(key, keyUnder, keyUpper, valueUnder, valueUpper);
         } else {

        }



    }  else
        return map.last();
*/
}


template <class T>
T list_max_value(const QList<T>& aList)
{
    typename QList<T>::const_iterator it = std::max_element(aList.cbegin(), aList.cend());
    if (it != aList.cend())
        return *it;
    return T(0);
}

template <class T>
T list_min_value(const QList<T>& aList)
{
    typename QList<T>::const_iterator it = std::min_element(aList.cbegin(), aList.cend());
    if (it != aList.cend())
        return *it;
    return T(0);
}

template <class T>
T vector_max_value(const QVector<T>& aVector)
{
    typename QVector<T>::const_iterator it = std::max_element(aVector.cbegin(), aVector.cend());
    if (it != aVector.cend())
        return *it;
    return T(0);
}

template <class T>
T vector_min_value(const QVector<T>& aVector)
{
    typename QVector<T>::const_iterator it = std::min_element(aVector.cbegin(), aVector.cend());
    if (it != aVector.cend())
        return *it;
    return T(0);
}

 template<typename T>
 inline const T inRange(const T minimum, const T value ,const T maximum)
{
    // see qBound in qglobal
    return std::max(minimum, std::min(maximum, value));
}

template <class U, class T>
T map_max_value(const QMap<U, T>& aMap)
{
    QMapIterator<U, T> iter(aMap);
    T max = iter.hasNext() ?  iter.next().value()  :  T(0) ;

    while (iter.hasNext()) {
        iter.next();
        max = qMax(max, iter.value());
    }
    return max;
}

template <class U, class T>
T map_min_value(const QMap<U, T>& aMap)
{
    QMapIterator<U, T> iter(aMap);
    T min = iter.hasNext() ?  iter.next().value()  :  T(0) ;

    while (iter.hasNext()) {
        iter.next();
        min = qMin(min, iter.value());
    }
    return min;
}

template <class U, class T>
T multimap_max_value(const QMultiMap<U, T>& aMap)
{
    QMultiMapIterator<U, T> iter(aMap);
    T max = iter.hasNext() ?  iter.next().value()  :  T(0) ;

    while (iter.hasNext()) {
        iter.next();
        max = qMax(max, iter.value());
    }
    return max;
}

template <class U, class T>
T multimap_min_value(const QMultiMap<U, T>& aMap)
{
    QMultiMapIterator<U, T> iter(aMap);
    T min = iter.hasNext() ?  iter.next().value()  :  T(0) ;

    while (iter.hasNext()) {
        iter.next();
        min = qMin(min, iter.value());
    }
    return min;
}
// --------------------------------
// can replace with std::accumulate(vector.begin(), vector.end(), T(0))
template<typename T>
T sum(const QVector<T>& vector)
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
T sum2(const QVector<T>& vector)
{
    T sum = 0;
    /*std::for_each(vector.cbegin(), vector.cend(), [&sum](T& v){
        sum += v * v;
    });*/
    for (const auto& v : vector) {
        sum += v * v;
    }
    return sum;
}

template<typename T>
T sumShifted(const QVector<T>& vector, const T& shift)
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

template<typename T>
T sum2Shifted(const QVector<T>& vector, const T& shift)
{
    T sum = 0;
    /*std::for_each(vector.cbegin(), vector.cend(), [&sum, &shift](T& v){
        sum += (v + shift) * (v + shift);
    });*/
     for (const auto& v : vector)
        sum += (v + shift) * (v + shift);


    return sum;
}

template <typename T>
T mean(const QVector<T>& vector)
{
    return sum(vector)/vector.size();
}

/**
 * @brief normalized sinc function
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
// --------------------------------
template<typename T>
QMap<T, T> normalize_map(const QMap<T, T>& aMap, const T max = 1)
{
    T max_value = map_max_value(aMap);

    QMap<T, T> result;
    // can be done with std::generate !!
    for( typename QMap<T, T>::const_iterator it = aMap.begin(); it != aMap.end(); ++it)
        result[it.key()] = (it.value() / max_value)*max;

    return result;
}

QVector<double> normalize_vector(const QVector<double>& aVector);
QVector<float> normalize_vector(const QVector<float>& aVector);

QVector<double> stretch_vector(const QVector<double>& aVector, const double from, const double to);
QVector<float> stretch_vector(const QVector<float>& aVector, const float from, const float to);

//QMap<double, double> normalize_map(const QMap<double, double>& aMap);
//QMap<float, float> normalize_map(const QMap<float, float>& aMap);

QMap<double, double> equal_areas(const QMap<double, double>& mapToModify, const QMap<double, double>& mapWithTargetArea);
QMap<float, float> equal_areas(const QMap<float, float>& mapToModify, const QMap<float, float>& mapWithTargetArea);
QMap<double, double> equal_areas(const QMap<double, double>& mapToModify, const double targetArea);
QMap<float, float> equal_areas(const QMap<float, float>& mapToModify, const float targetArea);

QVector<double> equal_areas(const QVector<double>& data, const double step, const double area);
QVector<float> equal_areas(const QVector<float>& data, const float step, const float area);


QMap<float, float> vector_to_map(const QVector<float>& data, const float min, const float max, const float step);
QMap<double, double> vector_to_map(const QVector<double>& data, const double min, const double max, const double step);
QMap<double, double> vector_to_map(const QVector<int> &data, const double min, const double max, const double step);

double vector_interpolate_idx_for_value(const double value, const QVector<double>& vector);
double vector_interpolate_idx_for_value(const double value, const std::vector<double>& vector);
float vector_interpolate_idx_for_value(const float value, const QVector<float>& vector);

double interpolate_value_from_curve(const double t, const QVector<double>& curve,const double curveTmin, const double curveTmax);
double interpolate_value_from_curve(const double x, const std::vector<double>& curve,const double Xmin, const double Xmax);

double map_area(const QMap<double, double>& map);
float map_area(const QMap<float, float>& map);
double map_area(const QMap<int, double>& density);
double map_area(const std::map<double, double>& density);

// const std::map<double, double> create_HPD(const QMap<double, double> &aMap, const double threshold); // Pour Qt5
const std::map<double, double> create_HPD(const QMap<double, double>& density, const double threshold = 95.);

const std::map<int, double> create_HPD(const QMap<int, double>& density, const double threshold = 95.);

QVector<double> vector_to_histo(const QVector<double>& dataScr, const double tmin, const double tmax, const int nbPts);

inline double diff_erf(double a, double b, double mu = 0., double sigma = 1.) {
    return 0.5*(erf((b-mu)/(sigma*M_SQRT2)) - erf((a-mu)/(sigma*M_SQRT2)));
}

// Fonction utilisée pour la courbe d'activité
std::vector<double> binomialeCurveByLog(const int n, const double alpha= .05, const int q_frac = 500);

std::vector<double> inverseCurve(const std::vector<double> Rp, const int x_frac = 500);

double findOnOppositeCurve (const double x, const std::vector<double> Gx);
#endif