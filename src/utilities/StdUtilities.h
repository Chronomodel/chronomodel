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
    QMapIterator<float, float> iter(aMap);
    float max = 0.f;
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
    QMapIterator<float, float> iter(aMap);
    float min = 0.f;
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
    QMapIterator<float, float> iter(aMap);
    float max = 0.f;
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




QList<float> create_histo(const QMap<long, float> &aMap, const int t_min, const int t_max);
QList<float> normalize_list(const QList<float>& aVector);
QVector<float> normalize_vector(const QVector<float>& aVector);
QMap<float, float> normalize_map(const QMap<float, float>& aMap);
QMap<float, float> equal_areas(const QMap<float, float>& mapToModify, const QMap<float, float>& mapWithTargetArea);
QMap<float, float> equal_areas(const QMap<float, float>& mapToModify, const float targetArea);
QVector<float> equal_areas(const QVector<float>& data, const float step, const float area);
QMap<float, float> vector_to_map(const QVector<float>& data, const float min, const float max, const float step);
float vector_interpolate_idx_for_value(const float value, const QVector<float>& vector);
float map_interpolate_key_for_value(const float value, const QMap<float, float>& aMap);
float map_interpolate_value_for_key(const float key, const QMap<float, float>& aMap);
QMap<float, float> vector_to_indexed_map(const QList<float>& aVector, const float minKey = 0, const float maxKey = 0);
QList<float> vector_shift_values_by(const QList<float>& aVector, const float v);
QList<float> sample_vector(const QList<float>& aVector, const int numValues);

QList<float> reshape_trace(const QList<float>& trace);
QMap<long, long> map_to_histogram(const QMap<long, float> &aMap, long aClasse);
QMap<long int,float> map_to_surface(const QMap<long, float> &aMap, long aClasse = 1);
//long int num_iterations_for_value(QMap<float, float> &aMap, const float value);
QMap<float, float> map_to_surface(const QMap<float, float>& aMap, float aClasse = 1);
QList<std::pair<float,float>> HPD_from_surface(const QMap<float, float>& aMap, float aClasse, float threshold);
const QMap<float, float> create_HPD(const QMap<float, float>& aMap, float aClasse = 1, float threshold = 0.95);

#endif
