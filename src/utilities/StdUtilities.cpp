#include "StdUtilities.h"
#include <cmath>
#include <ctgmath>
#include <cstdlib>
#include <iostream>
#include <random>
#include <algorithm>
#include <QDebug>

#include <fenv.h>

#include "AppSettings.h"

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
/**
 @brief this function transform a Qvector, than the maximum value is 1
 **/
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

/*QMap<double, double> equal_areas_old(const QMap<double, double>& mapToModify, const double targetArea)
{
    qDebug()<<"StdUtilities equal_areas begin"<<mapToModify.size();
    if(mapToModify.isEmpty())
        return QMap<double, double>();
 
    QMapIterator<double, double> iter(mapToModify);
    iter.next();
    double lastT = iter.key();
    double lastV = iter.value();
    double srcArea = 0.f;
    while(iter.hasNext())   {
        iter.next();
        double t = iter.key();
        double v = iter.value();
        if (lastV>0 && v>0) {
            srcArea += (lastV+v)/2 * (t - lastT);
        }
        // qDebug() << t << ", " << v;
        lastV = v;
        lastT = t;
    }
    
    double prop = targetArea / srcArea;
    
    QMap<double, double> result;
    QMapIterator<double, double> iter2(mapToModify);
    iter2.next();
    while(iter2.hasNext())
    {
        iter2.next();
        result[iter2.key()] = iter2.value() * prop;
    }
    
    
    return result;
}
 */

QMap<double, double> equal_areas(const QMap<double, double>& mapToModify, const double targetArea)
{
    if(mapToModify.isEmpty())
        return QMap<double, double>();
    
    QMap<double, double> result(mapToModify);
    
    QMap<double, double>::Iterator iter = result.begin();
    double t = iter.key();
    double v = iter.value();
    double lastT = t;
    double lastV = v;
    double srcArea = 0.f;
    // The map was sort with > by insertion
    while(iter!=result.end() )  {
        t = iter.key();
        v = iter.value();
        if (lastV>0 && v>0) {
            srcArea += (lastV+v)/2 * (t - lastT);
        }
        //qDebug() << t << ", " << v;
        lastV = v;
        lastT = t;
        ++iter;
    }
    double prop = targetArea / srcArea;
    
    iter = result.begin();
    while(iter!=result.end() ) {
        iter.value()= iter.value() * prop;
        
        ++iter;
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
    @brief  This function make a QMap which are a copy of the QMap aMap to obtain an percent of area
    @param threshold is in percent
 */
const QMap<double, double> create_HPD(const QMap<double, double>& aMap, double threshold)
{
    double areaTot = map_area(aMap);
    QMap<double, double> result;
    
    if (areaTot==threshold) {
        result = aMap;
        return result;
    }
    else {
    
        QMultiMap<double, double> inverted;
        QMapIterator<double, double> iter(aMap);

        while(iter.hasNext()) {
            iter.next();
            //  original code HL from v1.1
             double t = iter.key();
             double v = iter.value();

             inverted.insertMulti(v, t);

        }
    
        double areaSearched = areaTot * threshold / 100.;
    
    
        QMapIterator<double, double> iterInverted(inverted);
        iterInverted.toBack();
    
        double area         = 0.;
        bool areaFound      = false;
        bool symetryTested  = false;
        double lastV        = 0.;
    
        while(iterInverted.hasPrevious()) {
            iterInverted.previous();
            double t = iterInverted.value();
            double v = iterInverted.key();
        
            QMap<double, double> ::const_iterator iterMap = aMap.constFind(t);
        
            /*
                This part of code fix the case of irregular QMap when the step between keys are not the same
             and fix the calculus of the area on the extremum
             modif PhL 2015/05/20
             */
            if (iterMap.key() == t) { // it's mean : consFind(t) find the good key else iterMap = constEnd()
            
                double tPrev = 0.;
                double vPrev = 0.;
            
                if ( iterMap != aMap.constBegin() ) { // it's mean : iterMap is not the first item
                    tPrev = (iterMap-1).key();
                    vPrev = (iterMap-1).value();
                }
                if (vPrev>v) {
                    area   +=(v + vPrev)/2*(t - tPrev);
                }
        
                double tNext = 0.;
                double vNext = 0.;
                if (iterMap != aMap.constEnd() ) {
                    tNext = (iterMap+1).key();
                    vNext = (iterMap+1).value();
                }
     
                if (vNext>v) {
                    area   +=(v + vNext)/2*(tNext - t);
                }
       
            }
            //area += v; // original code HL from v1.1
        
            if(area < areaSearched) {
                result[t] = v;
            }
            else if(area > areaSearched) {
                    if(!areaFound) {
                        areaFound = true;
                        result[t] = v;
                    }
                    else  if(!symetryTested) {
                             symetryTested = true;
                             if(v == lastV) {
                                 result[t] = v;
                               }
                             else result[t] = 0;
                          }
                          else {
                              result[t] = 0;
                          }
                }
            lastV = v;
        }

        return result;
   }
}

double map_area(const QMap<double, double>& map)
{
    /* original code HL from v1.1
     QMapIterator<double, double> iter(map);
    double area = 0.;
    while(iter.hasNext())
    {
        iter.next();
        area += iter.value();
    }
    return area;
     */
    
    // Modif PhD  on 2015/05/20
    if(map.isEmpty())
        return 0;
    
    QMapIterator<double, double> iter(map);
    double srcArea = 0.f;
    iter.toFront();
    iter.next();

    double lastV = iter.value();
    double lastT = iter.key();
    
    while(iter.hasNext())
    {
        
        iter.next();
        double v = iter.value();
        double t = iter.key();
        if (lastV>0 && v>0) {
            srcArea += (lastV+v)/2 * (t-lastT);
        }
        lastV = v;
        lastT = t;
        
    }
       
    return srcArea;
}


