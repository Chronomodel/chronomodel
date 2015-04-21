#include "StdUtilities.h"
#include <cmath>
#include <ctgmath>
#include <cstdlib>
#include <iostream>
#include <random>
#include <algorithm>
#include <QDebug>

#include <fenv.h>

using namespace std;

// http://openclassrooms.com/forum/sujet/algorithme-de-levenshtein-50070
int compareStrings(const string &s1, const string &s2) {
    const size_t m = s1.size();
    const size_t n = s2.size();
    
    std::vector<int> prev(n+1);
    std::vector<int> crt(n+1);
    
    for(size_t j = 0; j <= n; j ++)
        crt[j] = j;
    
    for(size_t i = 1; i <= m; i ++) {
        crt.swap(prev);
        crt[0] = i; // par construction il semble inutile de procéder au moindre reset sur les autres colonnes
        for(size_t j = 1; j <= n; j++) {
            const int compt = (s1[i-1] == s2[i-1]) ? 0 : 1;
            crt[j] = min(min(prev[j]+1, crt[j-1]+1),prev[j-1]+compt);
#if 0
            // voire même ...
            const int compt = (s1[i-1] == s2[i-1]) ? 1 : 0;
            crt[j] = 1 + min(min(prev[j], crt[j-1]),prev[j-1]-compt);
            // ... qui me parait micropouillèmement plus optimisé
#endif
        }
    }
    
    return crt[n];
}

double safeExp(const double& x, int n)
{
    feclearexcept(FE_ALL_EXCEPT);
    double r = 0;
    try{
        r = exp(x);
        checkFloatingPointException();
    }catch(QString e){
        r = 1;
        for(int i=1; i<=n; ++i)
        {
            double factoriel = 1.;
            for(int j=1; j<=i; ++j)
                factoriel *= j;
            
            double elt = pow(x, i) / factoriel;
            int fe = fetestexcept(FE_ALL_EXCEPT);
            if(fe != 0)
                break;
            r += elt;
        }
        //qDebug() << "exp() using Taylor for exp(" << x << ") : " << r;
    }
    feclearexcept(FE_ALL_EXCEPT);
    return r;
}

double safeLog(const double& x, int n)
{
    feclearexcept(FE_ALL_EXCEPT);
    double r = 0;
    try{
        r = log(x);
        checkFloatingPointException();
    }catch(QString e){
        r = 1;
        for(int i=1; i<=n; ++i)
        {
            double product = 1.;
            for(int j=1; j<=i; ++j)
            {
                product *= (x - 1.);
            }
            if(i % 2)
                r -= product / i;
            else
                r += product / i;
        }
        //qDebug() << "log() using Taylor for log(" << x << ") : " << r;
    }
    feclearexcept(FE_ALL_EXCEPT);
    return r;
}

void checkFloatingPointException(const QString& infos)
{
    int fe = fetestexcept(FE_ALL_EXCEPT);
    
    QString message;
    
    if (fe & FE_DIVBYZERO) message = ("Divide by 0");
    //else if (fe & FE_INEXACT)   message = ("Inexact");
    else if (fe & FE_INVALID)   message = ("Invalid");
    else if (fe & FE_OVERFLOW)  message = ("Overflow");
    else if (fe & FE_UNDERFLOW) message = ("Underflow");
    
    if(!message.isEmpty())
    {
        message = QObject::tr("Floating point exception : ") + message + ". " + infos;
        throw message;
    }
    feclearexcept (FE_ALL_EXCEPT);
}

QVector<double> normalize_vector(const QVector<double>& aVector)
{
    QVector<double> histo;
    
    QVector<double>::const_iterator it = max_element(aVector.begin(), aVector.end());
    if(it != aVector.end())
    {
        double max_value = *it;
        for(QVector<double>::const_iterator it = aVector.begin(); it != aVector.end(); ++it)
        {
            histo.push_back((*it)/max_value);
        }
    }
    return histo;
}

QMap<double, double> normalize_map(const QMap<double, double>& aMap)
{
    double max_value = map_max_value(aMap);
    
    QMap<double, double> result;
    
    for(QMap<double, double>::const_iterator it = aMap.begin(); it != aMap.end(); ++it)
    {
        result[it.key()] = (it.value() / max_value);
    }
    return result;
}

QMap<double, double> equal_areas(const QMap<double, double>& mapToModify, const QMap<double, double>& mapWithTargetArea)
{
    if(mapToModify.isEmpty())
        return QMap<double, double>();
    
    QMapIterator<double, double> iter(mapWithTargetArea);
    double targetArea = 0.f;
    while(iter.hasNext())
    {
        iter.next();
        targetArea += iter.value();
    }
    return equal_areas(mapToModify, targetArea);
}

QMap<double, double> equal_areas(const QMap<double, double>& mapToModify, const double targetArea)
{
    if(mapToModify.isEmpty())
        return QMap<double, double>();
        
    QMapIterator<double, double> iter(mapToModify);
    
    iter.next();
    double lastT = iter.key();
    double lastV = iter.value();
    double srcArea = 0.f;
    
    //qDebug() << "------";
    
   /* while(iter.hasNext()) // original code HL
    {
        iter.next();
        double t = iter.key();
        double v = iter.value() * (t - lastT);
        qDebug() << t << ", " << v;
        srcArea += v;
        lastT = t;
    } */
    while(iter.hasNext())   {
        iter.next();
        double t = iter.key();
        double v = iter.value();// * (t - lastT);
        if (lastV>0 && v>0) {
            srcArea += (lastV+v)/2 * (t - lastT);
        }
        // qDebug() << t << ", " << v;
        lastV = v;
        lastT = t;
    }
    
    double prop = targetArea / srcArea;
   /* qDebug() << "Equal_areas prop = " << prop;
    qDebug() << "targetArea = " << targetArea;
    qDebug() << "srcArea = " << srcArea;
    */
    
    QMap<double, double> result;
    QMapIterator<double, double> iter2(mapToModify);
    while(iter2.hasNext())
    {
        iter2.next();
        result[iter2.key()] = iter2.value() * prop;
    }
    return result;
}

QVector<double> equal_areas(const QVector<double>& data, const double step, const double area)
{
    if(data.isEmpty())
        return QVector<double>();
    
    double srcArea = 0.f;
    double lastV = data[0];
   /* for(int i=0; i<data.size(); ++i) // original code HL
        srcArea += step * data[i];
    */
    
    for(int i=1; i<data.size(); ++i) {
        double v =data[i];
        
        if (lastV>0 && v>0) {
            srcArea += (lastV+v)/2 * step;
        }
       lastV = v;
    }
    double prop = area / srcArea;
    QVector<double> result;
    for(int i=0; i<data.size(); ++i)
        result.append(data[i] * prop);
    
    return result;
}

QMap<double, double> vector_to_map(const QVector<double>& data, const double min, const double max, const double step)
{
    QMap<double, double> map;
    int nbPts = 1 + (int)round((max - min) / step); // PhD step is not usefull, it's must be data.size/(max-min+1)
    for(int i=0; i<nbPts; ++i)
    {
        double t = min + i * step;
        if(i < data.size())
            map.insert(t, data[i]);
    }
    return map;
}

double vector_interpolate_idx_for_value(const double value, const QVector<double>& vector)
{
    // Dichotomie
    
    int idxInf = 0;
    int idxSup = vector.size() - 1;
    
    if(idxSup > idxInf)
    {
        do
        {
            int idxMid = idxInf + floor((idxSup - idxInf) / 2.f);
            double valueMid = vector[idxMid];
            
            if(value < valueMid)
                idxSup = idxMid;
            else
                idxInf = idxMid;
            
            //qDebug() << idxInf << ", " << idxSup;
            
        }while(idxSup - idxInf > 1);
        
        double valueInf = vector[idxInf];
        double valueSup = vector[idxSup];
        double prop = (value - valueInf) / (valueSup - valueInf);
        double idx = (double)idxInf + prop;
        
        return idx;
    }
    return 0;
}
/**
    @brief threshold is in percent
 */
const QMap<double, double> create_HPD(const QMap<double, double>& aMap, double threshold)
{
    QMultiMap<double, double> inverted;
    QMapIterator<double, double> iter(aMap);
    double areaTot = 0.f;
    while(iter.hasNext())
    {
        iter.next();
        double t = iter.key();
        double v = iter.value();
        
        areaTot += v;
        inverted.insertMulti(v, t);
    }
    
    double areaSearched = areaTot * threshold / 100.;
    
    QMap<double, double> result;
    iter = QMapIterator<double, double>(inverted);
    iter.toBack();
    
    double area = 0.;
    double finalArea = 0.;
    bool areaFound = false;
    bool symetryTested = false;
    double lastV = 0;
    
    while(iter.hasPrevious())
    {
        iter.previous();
        double t = iter.value();
        double v = iter.key();
        
        area += v;
        
        if(area < areaSearched)
        {
            result[t] = v;
            finalArea = area;
        }
        else if(area > areaSearched)
        {
            if(!areaFound)
            {
                areaFound = true;
                result[t] = v;
                finalArea = area;
            }
            else if(!symetryTested)
            {
                symetryTested = true;
                if(v == lastV)
                {
                    result[t] = v;
                    finalArea = area;
                }
                else
                    result[t] = 0;
            }
            else
            {
                result[t] = 0;
            }
        }
        lastV = v;
    }
    //double realThresh = finalArea / areaTot;
    return result;
}

double map_area(const QMap<double, double>& map)
{
    QMapIterator<double, double> iter(map);
    double area = 0.;
    while(iter.hasNext())
    {
        iter.next();
        area += iter.value();
    }
    return area;
}



