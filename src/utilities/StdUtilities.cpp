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
 @brief This function transforms a QVector turning its maximum value is 1 and adjusting other values accordingly
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

/**
 @brief This function transforms a QVector turning its minimum value to "from" and its maximum value is "to" and adjusting other values accordingly
 **/
QVector<double> stretch_vector(const QVector<double>& aVector, const double from, const double to)
{
    QVector<double> histo;
    QVector<double>::const_iterator it = aVector.constBegin();
    if(it != aVector.constEnd())
    {
        const double min = *(min_element(aVector.constBegin(), aVector.constEnd()));
        const double max = *(max_element(aVector.constBegin(), aVector.constEnd()));
        
        if(min < max)
        {
            for(QVector<double>::const_iterator it = aVector.constBegin(); it != aVector.constEnd(); ++it)
            {
                histo.push_back(from + (to - from) * (*it - min) / (max - min));
            }
        }
        else
        {
            // Just 1 value... set it to "from" (setting it to "to" could also be done though...)
            histo.push_back(to);
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
    
    QMap<double, double>::const_iterator cIter = result.cbegin();
    double t = cIter.key();
    double v = cIter.value();
    double lastT = t;
    double lastV = v;
    double srcArea = 0.f;
    // The map was sort with > by insertion
    while(cIter!=result.cend() )  {
        t = cIter.key();
        v = cIter.value();
        if (lastV>0 && v>0) {
            srcArea += (lastV+v)/2 * (t - lastT);
        }
        //qDebug() << t << ", " << v;
        lastV = v;
        lastT = t;
        ++cIter;
    }
    double prop = targetArea / srcArea;
    
    QMap<double, double>::iterator iter = result.begin();
    while(iter!=result.cend() ) {
        iter.value() *= prop;
        ++iter;
    }
    
    return result;
}

QVector<double> equal_areas(const QVector<double>& data, const double step, const double area)
{
    if(data.isEmpty())
        return QVector<double>();
    
    long double srcArea = 0;
    long double lastV = data.at(0);

    for(int i=1; i<data.size(); ++i) {
        const long double v =data.at(i);
        
        if (lastV>0 && v>0) {
            srcArea += (lastV+v)/2 * (long double)step;
        }
       lastV = v;
    }

    const long double invProp =srcArea / area;
    QVector<double> result;
    //for(int i=0; i<data.size(); ++i)
    //    result.append(data.at(i) / invProp);

    QVector<double>::const_iterator cIter = data.cbegin();
    while(cIter != data.cend() ) {
        result.append(*cIter / invProp);
        ++cIter;
    }



    return result;
}

QMap<double, double> vector_to_map(const QVector<double>& data, const double min, const double max, const double step)
{
    QMap<double, double> map;
    int nbPts = 1 + (int)round((max - min) / step); // step is not usefull, it's must be data.size/(max-min+1)
    for(int i=0; i<nbPts; ++i)
    {
        double t = min + i * step;
        if(i < data.size())
            map.insert(t, data.at(i));
    }
    return map;
}

/**
 * @brief This works only for strictly increasing functions!
 * @return interpolated index for a the given value. If value is lower than all vestor values, then 0 is returned. If value is upper than all vector values, then (vector.size() - 1) is returned.
 */
double vector_interpolate_idx_for_value(const double value, const QVector<double>& vector)
{
    int idxInf = 0;
    int idxSup = vector.size() - 1;

    if(value<vector.first()) return (double)idxInf;
    if(value>vector.last()) return (double)idxSup;

    // Dichotomie, we can't use indexOf because we don't know the step between each value in the Qvector
    
    if(idxSup > idxInf)
    {
        do
        {
            int idxMid = idxInf + floor((idxSup - idxInf) / 2.f);
            double valueMid = vector.at(idxMid);
            
            if(value < valueMid)
                idxSup = idxMid;
            else
                idxInf = idxMid;
            
        }while(idxSup - idxInf > 1);
        
        double valueInf = vector.at(idxInf);
        double valueSup = vector.at(idxSup);
        
        double prop = 0;
        // prevent valueSup=valueInf because in this case prop = NaN
        if(valueSup>valueInf) {
            prop = (value - valueInf) / (valueSup - valueInf);
        }
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
    const double areaTot = map_area(aMap);
    QMap<double, double> result;
    
    if (areaTot==threshold) {
        result = aMap;
        return result;
    }
    else {
        try {
            QMultiMap<double, double> inverted;
            QMap<double, double>::const_iterator cIter = aMap.cbegin();
            while(cIter != aMap.cend()) {
                 const double t = cIter.key();
                 const double v = cIter.value();
                 result[t] = 0; // important to init all the possible value
                 inverted.insertMulti(v, t);
                 ++cIter;
            }

            QMapIterator<double, double> iterInverted(inverted);

            double area         = 0.f;
            double areaSearched = areaTot * threshold / 100.;

            iterInverted.toBack();
        //--------------------
            while(iterInverted.hasPrevious()) {
                iterInverted.previous();
                const double t = iterInverted.value();
                const double v = iterInverted.key();

                QMap<double, double> ::const_iterator iterMap = aMap.constFind(t);

                /*
                    This part of code fix the case of irregular QMap when the step between keys are not the same
                 */
                if (iterMap.key() == t) { // meaning : constFind(t) find the good key else iterMap = constEnd()

                    if ( iterMap != aMap.constBegin() ) { // meaning : iterMap is not the first item
                        const double vPrev = (iterMap-1).value();
                        if (vPrev>=v) {
                            const double tPrev = (iterMap-1).key();
                            area   +=(v + vPrev)/2*(t - tPrev);
                        }
                    }

                    if (iterMap != aMap.constEnd() ) {
                        const double vNext = (iterMap+1).value();
                        if (vNext>v) {
                            const double tNext = (iterMap+1).key();
                            area   +=(v + vNext)/2*(tNext - t);
                        }
                    }

                }

                if(iterInverted.hasPrevious() &&  (iterInverted.peekPrevious().key()==v) ) {
                    result[t] = v;
                }
                 else {
                    if(area < areaSearched) {
                        result[t] = v;
                    }
                    else if(area > areaSearched) {
                        return result;
                    }
                }
            }

            return result;
       }
       catch (std::exception const & e) {
            qDebug()<< "in stdUtilities::create_HPD() Error"<<e.what();
            return aMap;
       }


    }
}

double map_area(const QMap<double, double>& map)
{
    if(map.isEmpty())
        return 0;
    
    QMap<double, double>::const_iterator cIter = map.cbegin();
    double srcArea = 0.f;

    double lastV = cIter.value();
    double lastT = cIter.key();
    
    while(cIter != map.cend())
    {
        const double v = cIter.value();
        const double t = cIter.key();
        if (lastV>0 && v>0) {
            srcArea += (lastV+v)/2 * (t-lastT);
        }
        lastV = v;
        lastT = t;
        ++cIter;
    }
       
    return srcArea;
}


