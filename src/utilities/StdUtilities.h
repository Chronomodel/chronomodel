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

#define DEBUG
#ifdef DEBUG
#include <iostream>
#endif
#ifndef M_PI
#define M_PI 3.14159265358979323846 //usefull to Windows
#endif


int compareStrings(const std::string &s1, const std::string &s2);

QString doubleInStrDate(double date);
QString dateFormat();
double dateInDouble(double date);
double doubleInDate(double value);


double safeExp(const double& x, int n = 10);
double safeLog(const double& x, int n = 5);

void checkFloatingPointException(const QString& infos = QString());

template <typename T>
T interpolate(const T& x, const T& x1, const T& x2, const T& y1, const T& y2)
{
    return (y1 + (y2 - y1) * (x - x1) / (x2 - x1));
}

template <class T>
T vector_max_value(const QList<T>& aVector)
{
    typename QList<T>::const_iterator it = std::max_element(aVector.begin(), aVector.end());
    if(it != aVector.end())
        return *it;
    return 0;
}

template <class T>
T vector_min_value(const QList<T>& aVector)
{
    typename QList<T>::const_iterator it = std::min_element(aVector.begin(), aVector.end());
    if(it != aVector.end())
        return *it;
    return 0;
}

template <class T>
T vector_max_value(const QVector<T>& aVector)
{
    typename QVector<T>::const_iterator it = std::max_element(aVector.begin(), aVector.end());
    if(it != aVector.end())
        return *it;
    return 0;
}

template <class T>
T vector_min_value(const QVector<T>& aVector)
{
    typename QVector<T>::const_iterator it = std::min_element(aVector.begin(), aVector.end());
    if(it != aVector.end())
        return *it;
    return 0;
}

 template<typename T>
 const T inRange(const T minimum, const T value ,const T maximum)
{
    // voir qBound dans qglobal
    return std::max(minimum, std::min(maximum, value));
}

template <class U, class T>
T map_max_value(const QMap<U, T>& aMap)
{
    QMapIterator<double, double> iter(aMap);
    double max = 0.f;
    if(iter.hasNext())
    {
        iter.next();
        max = iter.value();
    }
    while(iter.hasNext())
    {
        iter.next();
        max = qMax(max, iter.value());
    }
    return max;
}

template <class U, class T>
T map_min_value(const QMap<U, T>& aMap)
{
    QMapIterator<double, double> iter(aMap);
    double min = 0.f;
    if(iter.hasNext())
    {
        iter.next();
        min = iter.value();
    }
    while(iter.hasNext())
    {
        iter.next();
        min = qMin(min, iter.value());
    }
    return min;
}

QVector<double> normalize_vector(const QVector<double>& aVector);
QMap<double, double> normalize_map(const QMap<double, double>& aMap);
QMap<double, double> equal_areas(const QMap<double, double>& mapToModify, const QMap<double, double>& mapWithTargetArea);
QMap<double, double> equal_areas(const QMap<double, double>& mapToModify, const double targetArea);
QVector<double> equal_areas(const QVector<double>& data, const double step, const double area);
QMap<double, double> vector_to_map(const QVector<double>& data, const double min, const double max, const double step);
double vector_interpolate_idx_for_value(const double value, const QVector<double>& vector);

double map_area(const QMap<double, double>& map);
const QMap<double, double> create_HPD(const QMap<double, double>& aMap, double threshold);

#endif
