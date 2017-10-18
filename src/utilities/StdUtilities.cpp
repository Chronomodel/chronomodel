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
namespace //anonyme
{
    struct fillrow
    {
        fillrow() : mVal() {}
        int operator()() { return mVal++; }
    protected:
        int mVal;
    };
}

int compareStrings(const string &s1, const string &s2)
{
    const size_t m ( s1.size());
    const size_t n ( s2.size());
    int*  rawPtr ( reinterpret_cast<int*>( alloca( (n+1)*2*sizeof(int))));
    
    int*  prev ( rawPtr);
    int*  crt ( rawPtr + n+1);
    
    std::generate( crt, crt + n+1, fillrow() );
    
    for (size_t i = 1; i <= m; i ++) {
        std::swap( prev, crt);
        crt[0] = (int)i; // par construction il semble inutile de procéder au moindre reset sur les autres colonnes
        for (size_t j = 1; j <= n; j++) {
            const int compt = (s1[i-1] == s2[i-1]) ? 0 : 1;
            crt[j] = min(min(prev[j]+1, crt[j-1]+1),prev[j-1]+compt);
        }
    }
   return crt[n];
}

/**
 * @brief eraseZeroAtLeft erase useless 0 at the end of a string comming from a number
 * @param str
 * @return
 */
std::string removeZeroAtRight(std::string str)
{
    // controle if str has decimal
    if (str.find('.') == 0  || str.find(',') == 0)
        return str;

    // in the declaration str is pass as "a copy" so we can modify it
    if (str.back() == '0')
        do {
            str.pop_back();
        } while (str.back() == '0');

    if (str.back() == '.' ||str.back() == ',')
        str.pop_back();

    return str;
}



double safeExp(const double& x, int n)
{
    feclearexcept(FE_ALL_EXCEPT);
    double r = 0.;
    try {
        r = exp(x);
        checkFloatingPointException();
    } catch (QString e) {
        r = 1.;
        for (int i=1; i<=n; ++i) {
            double factoriel = 1.;
            for (int j=1; j<=i; ++j)
                factoriel *= j;
            
            double elt = pow(x, i) / factoriel;
            int fe = fetestexcept(FE_ALL_EXCEPT);
            if (fe != 0)
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
    double r = 0.;
    try {
        r = log(x);
        checkFloatingPointException();
    } catch (QString e){
        r = 1.;
        for (int i=1; i<=n; ++i)  {
            double product = 1.;
            for (int j=1; j<=i; ++j)
                product *= (x - 1.);

            if (i % 2)
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
    
    if (!message.isEmpty()) {
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
    if (it != aVector.end()) {
        double max_value = *it;

        for (auto&& value : aVector)
            histo.push_back(value/max_value);
    }
    return histo;
}
QVector<float> normalize_vector(const QVector<float>& aVector)
{
    QVector<float> histo;

    QVector<float>::const_iterator it = max_element(aVector.begin(), aVector.end());
    if (it != aVector.end()){
        float max_value = *it;
        for (QVector<float>::const_iterator it = aVector.begin(); it != aVector.end(); ++it)
            histo.push_back((*it)/max_value);

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
    if (it != aVector.constEnd()) {
        const double min = *(min_element(aVector.constBegin(), aVector.constEnd()));
        const double max = *(max_element(aVector.constBegin(), aVector.constEnd()));
        
        if (min < max) {
            for (QVector<double>::const_iterator it = aVector.constBegin(); it != aVector.constEnd(); ++it)
                histo.push_back(from + (to - from) * (*it - min) / (max - min));

        } else // Just 1 value... set it to "from" (setting it to "to" could also be done though...)
            histo.push_back(to);

    }
    return histo;
}
QVector<float> stretch_vector(const QVector<float>& aVector, const float from, const float to)
{
    QVector<float> histo;
    QVector<float>::const_iterator it = aVector.constBegin();
    if (it != aVector.constEnd()) {
        const float min = *(min_element(aVector.constBegin(), aVector.constEnd()));
        const float max = *(max_element(aVector.constBegin(), aVector.constEnd()));

        if (min < max) {
            for (QVector<float>::const_iterator it = aVector.constBegin(); it != aVector.constEnd(); ++it)
                histo.push_back(from + (to - from) * (*it - min) / (max - min));

        } else {
            // Just 1 value... set it to "from" (setting it to "to" could also be done though...)
            histo.push_back(to);
        }
    }
    return histo;
}

QMap<double, double> equal_areas(const QMap<double, double>& mapToModify, const QMap<double, double>& mapWithTargetArea)
{
    if (mapToModify.isEmpty())
        return QMap<double, double>();
    
    QMapIterator<double, double> iter(mapWithTargetArea);
    double targetArea = 0.;
    while (iter.hasNext()) {
        iter.next();
        targetArea += iter.value();
    }
    return equal_areas(mapToModify, targetArea);
}
QMap<float, float> equal_areas(const QMap<float, float>& mapToModify, const QMap<float, float>& mapWithTargetArea)
{
    if (mapToModify.isEmpty())
        return QMap<float, float>();

    QMapIterator<float, float> iter(mapWithTargetArea);
    float targetArea = 0.f;
    while (iter.hasNext())  {
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
    if (mapToModify.isEmpty())
        return QMap<double, double>();
    
    QMap<double, double> result(mapToModify);
    
    QMap<double, double>::const_iterator cIter = result.cbegin();
    double t = cIter.key();
    double v = cIter.value();
    double lastT = t;
    double lastV = v;
    double srcArea = 0.;
    // The map was sort with > by insertion
    while (cIter!=result.cend() )  {
        t = cIter.key();
        v = cIter.value();

        if (lastV>0 && v>0)
            srcArea += (lastV+v)/2 * (t - lastT);

        //qDebug() << t << ", " << v;
        lastV = v;
        lastT = t;
        ++cIter;
    }
    double prop = targetArea / srcArea;
    
    QMap<double, double>::iterator iter = result.begin();
    while (iter!=result.cend() ) {
        iter.value() *= prop;
        ++iter;
    }
    
    return result;
}
QMap<float, float> equal_areas(const QMap<float, float>& mapToModify, const float targetArea)
{
    if (mapToModify.isEmpty())
        return QMap<float, float>();

    QMap<float, float> result(mapToModify);

    QMap<float, float>::const_iterator cIter = result.cbegin();
    float t = cIter.key();
    float v = cIter.value();
    float lastT = t;
    float lastV = v;
    float srcArea = 0.f;
    // The map was sort with > by insertion
    while (cIter!=result.cend() )  {
        t = cIter.key();
        v = cIter.value();
        if (lastV>0 && v>0)
            srcArea += (lastV+v)/2.f * (t - lastT);

        //qDebug() << t << ", " << v;
        lastV = v;
        lastT = t;
        ++cIter;
    }
    float prop = targetArea / srcArea;

    QMap<float, float>::iterator iter = result.begin();
    while (iter!=result.cend() ) {
        iter.value() *= prop;
        ++iter;
    }

    return result;
}

QVector<double> equal_areas(const QVector<double>& data, const double step, const double area)
{
    if(data.isEmpty())
        return QVector<double>();
    
    long double srcArea (0.);
    long double lastV = data.at(0);

    //for (int i=1; i<data.size(); ++i) {
    for (auto&& value : data) {
        const long double v = value;//data.at(i);
        
        if (lastV>0 && v>0)
            srcArea += (lastV+v)/2. * (long double)step;

       lastV = v;
    }

    const long double invProp =srcArea / area;
    QVector<double> result;
    //for(int i=0; i<data.size(); ++i)
    //    result.append(data.at(i) / invProp);

    QVector<double>::const_iterator cIter = data.cbegin();
    while (cIter != data.cend() ) {
        result.append(*cIter / invProp);
        ++cIter;
    }

    return result;
}

QVector<float> equal_areas(const QVector<float>& data, const float step, const float area)
{
    if (data.isEmpty())
        return QVector<float>();

    long double srcArea (0.);
    long double lastV = data.at(0);

    //for (int i=1; i<data.size(); ++i) {
    //    const long double v = data.at(i);
    for (auto&& value : data) {
        const long double v = value;

        if (lastV>0. && v>0.)
            srcArea += (lastV+v)/2. * (long double)step;

       lastV = v;
    }

    const long double invProp =srcArea / area;
    QVector<float> result;
    //for(int i=0; i<data.size(); ++i)
    //    result.append(data.at(i) / invProp);

    QVector<float>::const_iterator cIter = data.cbegin();
    while (cIter != data.cend() ) {
        result.append((float)(*cIter / invProp));
        ++cIter;
    }

    return result;
}


QMap<double, double> vector_to_map(const QVector<double>& data, const double min, const double max, const double step)
{
    Q_ASSERT(max>=min && !data.isEmpty());
    QMap<double, double> map;
    if (min == max)
        map.insert(min, data.at(0));

    else {
        const int nbPts = 1 + (int)round((max - min) / step); // step is not usefull, it's must be data.size/(max-min+1)

        for (int i=0; i<nbPts; ++i) {
            double t = min + i * step;

            if (i < data.size())
                map.insert(t, data.at(i));
        }
    }

    return map;
}

QMap<double, double> vector_to_map(const QVector<int>& data, const double min, const double max, const double step)
{
    Q_ASSERT(max>=min && !data.isEmpty());
    QMap<double, double> map;
    if (min == max)
        map.insert(min, data.at(0));

    else {
        const int nbPts = 1 + (int)round((max - min) / step); // step is not usefull, it's must be data.size/(max-min+1)

        for (int i=0; i<nbPts; ++i) {
            double t = min + i * step;

            if (i < data.size())
                map.insert(t,  (double) data.at(i));

        }
    }
    return map;
}

QMap<float, float> vector_to_map(const QVector<float>& data, const float min, const float max, const float step)
{
    Q_ASSERT(max>=min && !data.isEmpty());
    QMap<float, float> map;
    if (min == max)
        map.insert(min, data.at(0));

    else {
        const int nbPts = 1 + (int)round((max - min) / step); // step is not usefull, it's must be data.size/(max-min+1)
        for (int i=0; i<nbPts; ++i) {
            float t = min + i * step;
            if (i < data.size())
                map.insert(t, data.at(i));
        }
    }
    return map;
}
/**
 * @brief This works only for strictly increasing functions!
 * @return interpolated index for a the given value. If value is lower than all vestor values, then 0 is returned. If value is upper than all vector values, then (vector.size() - 1) is returned.
 */
double vector_interpolate_idx_for_value(const double value, const QVector<double>& vector)
{
    int idxInf (0);
    int idxSup = vector.size() - 1;

    if (value<vector.first())
        return (double)idxInf;

    if (value>vector.last())
        return (double)idxSup;

    // Dichotomie, we can't use indexOf because we don't know the step between each value in the Qvector
    
    if (idxSup > idxInf) {
        do
        {
            const int idxMid = idxInf + floor((idxSup - idxInf) / 2.);
            const double valueMid = vector.at(idxMid);
            
            if (value < valueMid)
                idxSup = idxMid;
            else
                idxInf = idxMid;
            
        } while (idxSup - idxInf > 1);
        
        const double valueInf = vector.at(idxInf);
        const double valueSup = vector.at(idxSup);
        
        double prop = 0.;
        // prevent valueSup=valueInf because in this case prop = NaN
        if (valueSup>valueInf)
            prop = (value - valueInf) / (valueSup - valueInf);

        const double idx = (double)idxInf + prop;
        
        return idx;
    }

    return 0;
}
float vector_interpolate_idx_for_value(const float value, const QVector<float>& vector)
{
    int idxInf = 0;
    int idxSup = vector.size() - 1;

    if (value<vector.first())
        return (float)idxInf;

    if  (value>vector.last())
        return (float)idxSup;

    // Dichotomie, we can't use indexOf because we don't know the step between each value in the Qvector

    if (idxSup > idxInf) {
        do {
            const int idxMid = idxInf + floor((idxSup - idxInf) / 2.f);
            const float valueMid = vector.at(idxMid);

            if (value < valueMid)
                idxSup = idxMid;
            else
                idxInf = idxMid;

        } while (idxSup - idxInf > 1);

        const float valueInf = vector.at(idxInf);
        const float valueSup = vector.at(idxSup);

        float prop = 0.f;
        // prevent valueSup=valueInf because in this case prop = NaN
        if(valueSup>valueInf)
            prop = (value - valueInf) / (valueSup - valueInf);

        const float idx = (float)idxInf + prop;

        return idx;
    }

    return 0.f;
}
/**
    @brief  This function make a QMap which are a copy of the QMap aMap to obtain an percent of area
    @brief  to define a area we need at least 2 value in the map
    @param threshold is in percent
 */
const QMap<double, double> create_HPD(const QMap<double, double>& aMap, const double threshold)
{
    QMap<double, double> result = QMap<double, double>();

    if (aMap.size() < 2)  // in case of only one value (e.g. a bound fixed) or no value
        return result;


    const double areaTot = map_area(aMap);
    if (areaTot==threshold) {
        result = aMap;
        return result;
        
    } else {
        try {
            QMultiMap<double, double> inverted;
            QMap<double, double>::const_iterator cIter = aMap.cbegin();
            while (cIter != aMap.cend()) {
                 const double t = cIter.key();
                 const double v = cIter.value();
                 result[t] = 0.; // important to init all the possible value
                 inverted.insertMulti(v, t);
                 ++cIter;
            }

            QMapIterator<double, double> iterInverted(inverted);

            double area (0.);
            double areaSearched = areaTot * threshold / 100.;

            iterInverted.toBack();
        //--------------------
            while (iterInverted.hasPrevious()) {
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
                            area += (v + vPrev)/2*(t - tPrev);
                        }
                    }

                    if (iterMap != aMap.constEnd() ) {
                        const double vNext = (iterMap+1).value();
                        if (vNext>v) {
                            const double tNext = (iterMap+1).key();
                            area += (v + vNext)/2*(tNext - t);
                        }
                    }

                }

                if (iterInverted.hasPrevious() &&  (iterInverted.peekPrevious().key()==v) )
                    result[t] = v;
                else {
                    
                    if (area < areaSearched)
                        result[t] = v;

                    else if (area > areaSearched)
                        return result;
                    
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
    if (map.size()<2)
        return 0.0;
    
    QMap<double, double>::const_iterator cIter = map.cbegin();
    double srcArea (0.);

    double lastV = cIter.value();
    double lastT = cIter.key();
    
    while (cIter != map.cend())  {
        const double v = cIter.value();
        const double t = cIter.key();
        if (lastV>0 && v>0)
            srcArea += (lastV+v)/2 * (t-lastT);

        lastV = v;
        lastT = t;
        ++cIter;
    }
       
    return srcArea;
}
float map_area(const QMap<float, float>& map)
{
    if (map.size()<2)
        return 0.f;

    QMap<float, float>::const_iterator cIter = map.cbegin();
    float srcArea = 0.f;

    float lastV = cIter.value();
    float lastT = cIter.key();

    while (cIter != map.cend()) {
        const float v = cIter.value();
        const float t = cIter.key();
        if (lastV>0 && v>0)
            srcArea += (lastV+v)/2 * (t-lastT);

        lastV = v;
        lastT = t;
        ++cIter;
    }

    return srcArea;
}

QVector<double> vector_to_histo(const QVector<double>& dataScr, const double tmin, const double tmax, const int nbPts)
{

    QVector<double> histo;
    histo.reserve(nbPts);
    histo.fill(0.,nbPts);
    const double delta = (tmax - tmin) / (nbPts - 1);

    const double denum = dataScr.size();

    for (QVector<double>::const_iterator iter = dataScr.cbegin(); iter != dataScr.cend(); ++iter) {
        const double t = *iter;
        
        const double idx = (t - tmin) / delta;
        const double idx_under = floor(idx);
        const double idx_upper = idx_under + 1.;
        
        const double contrib_under = (idx_upper - idx) / denum;
        const double contrib_upper = (idx - idx_under) / denum;
        
        if (std::isinf(contrib_under) || std::isinf(contrib_upper))
            qDebug() << "StdUtilities::vector_to_histo() : infinity contrib!";

        if (idx_under < 0 || idx_under >= nbPts || idx_upper < 0 || idx_upper > nbPts)
            qDebug() << "StdUtilities::vector_to_histo() : Wrong index";


        if (idx_under < nbPts)
            histo[(int)idx_under] += contrib_under;

        if (idx_upper < nbPts) // This is to handle the case when matching the last point index !
            histo[(int)idx_upper] += contrib_upper;
    }

    bool bEmpty = true;
    for (QVector<double>::const_iterator iter = histo.cbegin();( iter != histo.cend())&& bEmpty; ++iter) {
         if (*iter> 0)
             bEmpty= false;
    }
    if (bEmpty)
        qDebug()<<"in vector_to_histo histo is empty !!!!";

    return histo;
}
