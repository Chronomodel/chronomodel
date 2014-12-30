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
#define M_PI 3.14159265358979323846 //utile pour PhD
#endif

template <typename T>
T interpolate(const T& x, const T& x1, const T& x2, const T& y1, const T& y2)
{
    return (y1 + (y2 - y1) * (x - x1) / (x2 - x1));
}

template <typename T,typename U>
long int num_iterations_for_value(const QMap<T, U>& aMap, const U value)
{
    typename QMap<T,U>::const_iterator it=aMap.begin();
    long int nb=0;
    while(it!=aMap.end())
    {
       nb=(it.value() == value ? nb+1 : nb);
       ++it;
    }
    return nb;
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

template <class U, class T>
QMap<T, U> map_reverse(const QMap<U, T>& aMap)
{
    QMapIterator<U, T> iter(aMap);
    QMap<T, U> reversed;
    while(iter.hasNext())
    {
        iter.next();
        reversed[iter.value()] = iter.key();
    }
    return reversed;
}

/**
  * \ brief Return the value of the maximum key in a map
  */
template <class U, class T>
U map_max_key(const QMap<U, T>& aMap)
{
    if(aMap.isEmpty())
        return 0;
    return aMap.lastKey();
}

template <class U, class T>
U map_min_key(const QMap<U, T>& aMap)
{
    if(aMap.isEmpty())
        return 0;
    return aMap.firstKey();
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

    /*if(aMap.isEmpty())
        return 0;
    
    QMap<T, U> reversed = map_reverse(aMap);
    return reversed.firstKey();*/
}

// get the key of the maximum value
template <class U, class T>
T map_max_value_key(const QMap<U, T>& aMap)
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
    
    /*if(aMap.isEmpty())
        return 0;
    
    QMap<T, U> reversed = map_reverse(aMap);
    return reversed.last();*/
}

template <class T, class U>
void remove_value_at(QMap<T, U>& container, const T key)
{
    //typename QMap<T, U>::iterator it = container.find(key);
    container.erase(key);
}

template <class T>
void remove_value_at(QList<T>& container, const int index)
{
    typename QList<T>::iterator it = container.find(index);
    container.erase(it);
    //container.erase(std::remove(container.begin(), container.end(), index), container.end());
}

template <class T, class U>
typename QMap<T, U>::const_iterator map_lower_bound(const QMap<T, U>& aMap, const T& key)
{
    typename QMap<T, U>::const_iterator it;
    for(it = aMap.begin(); it != aMap.end(); ++it)
    {
        if((*it).first > key && it != aMap.begin())
        {
            return --it;
        }
    }
    return aMap.end();
}

template <class T, class U>
typename QMap<T, U>::const_iterator map_upper_bound(const QMap<T, U>& aMap, const T& key)
{
    typename QMap<T, U>::const_iterator it;
    for(it = aMap.begin(); it != aMap.end(); ++it)
    {
        if((*it).first > key)
        {
            return it;
        }
    }
    return aMap.end();
}




QList<double> create_histo(const QMap<long, double> &aMap, const int t_min, const int t_max);
QList<double> normalize_list(const QList<double>& aVector);
QVector<double> normalize_vector(const QVector<double>& aVector);
QMap<double, double> normalize_map(const QMap<double, double>& aMap);
QMap<double, double> equal_areas(const QMap<double, double>& mapToModify, const QMap<double, double>& mapWithTargetArea);
QMap<double, double> equal_areas(const QMap<double, double>& mapToModify, const double targetArea);
QVector<double> equal_areas(const QVector<double>& data, const double step, const double area);
QMap<double, double> vector_to_map(const QVector<double>& data, const double min, const double max, const double step);
double vector_interpolate_idx_for_value(const double value, const QVector<double>& vector);
double map_interpolate_key_for_value(const double value, const QMap<double, double>& aMap);
double map_interpolate_value_for_key(const double key, const QMap<double, double>& aMap);
QMap<double, double> vector_to_indexed_map(const QList<double>& aVector, const double minKey = 0, const double maxKey = 0);
QList<double> vector_shift_values_by(const QList<double>& aVector, const double v);
QList<double> sample_vector(const QList<double>& aVector, const int numValues);

QList<double> reshape_trace(const QList<double>& trace);
QMap<long, long> map_to_histogram(const QMap<long, double> &aMap, long aClasse);
QMap<long int,double> map_to_surface(const QMap<long, double> &aMap, long aClasse = 1);
//long int num_iterations_for_value(QMap<double, double> &aMap, const double value);
QMap<double, double> map_to_surface(const QMap<double, double>& aMap, double aClasse = 1);
QList<std::pair<double,double>> HPD_from_surface(const QMap<double, double>& aMap, double aClasse, double threshold);
const QMap<double, double> create_HPD(const QMap<double, double>& aMap, double aClasse = 1, double threshold = 0.95);

#endif
