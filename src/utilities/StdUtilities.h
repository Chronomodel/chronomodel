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
    auto lIter = map.lowerBound(key);
    T valueUpper = lIter.value();
    T keyUpper = lIter.key();

    if (key<=keyUpper)
        return valueUpper;

    else if (lIter!=map.end()) {
        T valueUnder = (lIter - 1).value();
        T keyUnder =(lIter - 1).key();

        return interpolate(key, keyUnder, keyUpper, valueUnder, valueUpper);
    }
    else
        return map.last();

}


template <class T>
T vector_max_value(const QList<T>& aVector)
{
    typename QList<T>::const_iterator it = std::max_element(aVector.cbegin(), aVector.cend());
    if (it != aVector.cend())
        return *it;
    return T(0);
}

template <class T>
T vector_min_value(const QList<T>& aVector)
{
    typename QList<T>::const_iterator it = std::min_element(aVector.cbegin(), aVector.cend());
    if (it != aVector.cend())
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
 const T inRange(const T minimum, const T value ,const T maximum)
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

// --------------------------------
// can replace with std::accumulate(vector.begin(), vector.end(), T(0))
template<typename T>
T sum(const QVector<T>& vector)
{
    T s = 0;
   /* std::for_each(vector.begin(), vector.end(), [&s](T& v){
        s += v;
    });*/
    for (auto&& v : vector) {
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
    for (auto&& v : vector) {
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
     for (auto&& v : vector) {
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
     for (auto&& v : vector)
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

double vector_interpolate_idx_for_value(const double value, const QVector<double> &vector);
float vector_interpolate_idx_for_value(const float value, const QVector<float> &vector);

double map_area(const QMap<double, double>& map);
float map_area(const QMap<float, float>& map);
const QMap<double, double> create_HPD(const QMap<double, double> &aMap, const double threshold);
QVector<double> vector_to_histo(const QVector<double>& dataScr, const double tmin, const double tmax, const int nbPts);
#endif
