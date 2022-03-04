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

#include "StdUtilities.h"
#include "AppSettings.h"

#include <ctgmath>
#include <cstdlib>
#include <iostream>
#include <fenv.h>

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
    while (iter!=result.end() ) {
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
    while (iter!=result.end() ) {
        iter.value() *= prop;
        ++iter;
    }

    return result;
}

QVector<double> equal_areas(const QVector<double>& data, const double step, const double area)
{
    if(data.isEmpty())
        return QVector<double>();

    long double srcArea (0.l);
    long double lastV = data.at(0);

    if (data.size()>3) {
        for (int i=1; i<data.size(); ++i) {
            const long double v = data.at(i);
            if (lastV>0 && v>0)
                srcArea += (lastV+v)/2.l * (long double)step;

           lastV = v;
        }
    } else { // append when the function is just defined in one step of the study period
        for (auto d: data)
                srcArea += d;
    }

    const long double invProp =srcArea / area;
    QVector<double> result;

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

    long double srcArea (0.l);
    long double lastV = data.at(0);

    for (auto&& value : data) {
        const long double v = value;

        if (lastV>0.l && v>0.l)
            srcArea += (lastV+v)/2.l * (long double)step;

       lastV = v;
    }

    const long double invProp =srcArea / area;
    QVector<float> result;

    QVector<float>::const_iterator cIter = data.cbegin();
    while (cIter != data.cend() ) {
        result.append(float (*cIter / invProp));
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
        const int nbPts = 1 + int (round((max - min) / step)); // step is not usefull, it's must be data.size/(max-min+1)
        double t;
        for (int i = 0; i<nbPts; ++i) {
            t = min + i * step;

            if (i < data.size())
                map.insert(t, data.at(i));
        }
    }

    return map;
}

QMap<double, double> vector_to_map(const QVector<int>& data, const double min, const double max, const double step)
{
    Q_ASSERT(max>=min && !data.isEmpty());
    QMap<double, double> map ;
    if (min == max)
        map.insert(min, data.at(0));

    else {
        const int nbPts = 1 + (int)round((max - min) / step); // step is not usefull, it's must be data.size/(max-min+1)
        double t;
        for (int i = 0; i<nbPts; ++i) {
             t = min + i * step;

            if (i < data.size())
                map.insert(t,  double (data.at(i)));

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
        const int nbPts = 1 + int (round((max - min) / step)); // step is not usefull, it's must be data.size/(max-min+1)
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
 * @return interpolated index for a the given value. If value is lower than all vector values, then 0 is returned. If value is upper than all vector values, then (vector.size() - 1) is returned.
 */
double vector_interpolate_idx_for_value(const double value, const QVector<double>& vector)
{
    int idxInf (0);
    int idxSup = vector.size() - 1;

    if (value<vector.first())
        return double (idxInf);

    if (value>vector.last())
        return double (idxSup);

    // Dichotomie, we can't use indexOf because we don't know the step between each value in the Qvector

    if (idxSup > idxInf) {
        do
        {
            const int idxMid = idxInf + int (floor((idxSup - idxInf) / 2.));
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

        const double idx = double (idxInf) + prop;

        return idx;
    }

    return 0;
}
float vector_interpolate_idx_for_value(const float value, const QVector<float>& vector)
{
    int idxInf = 0;
    int idxSup = vector.size() - 1;

    if (value<vector.first())
        return float (idxInf);

    if  (value>vector.last())
        return  float (idxSup);

    // Dichotomie, we can't use indexOf because we don't know the step between each value in the Qvector

    if (idxSup > idxInf) {
        do {
            const int idxMid = idxInf + int (floor((idxSup - idxInf) / 2.));
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
        if (valueSup>valueInf)
            prop = (value - valueInf) / (valueSup - valueInf);

        const float idx = float (idxInf) + prop;

        return idx;
    }

    return 0.f;
}

/**
 * @brief interpolate_value_from_curve Allows you to find the value associated with a time in a curve.
 * @param t Time for which we are looking for value
 * @param curve
 * @param curveTmin Time corresponding to index 0
 * @param curveTmax Time corresponding to index curve.size()-1
 * @return
 */
double interpolate_value_from_curve(const double t, const QVector<double> & curve,const double curveTmin, const double curveTmax)
{
     // We need at least two points to interpolate
    if (curve.size() < 2 || t <= curveTmin) {
        return curve.first();

    } else if (t >= curveTmax) {
        return curve.last();
    }

    const double prop = (t - curveTmin) / (curveTmax - curveTmin);
    const double idx = prop * (curve.size() - 1); // tricky : if (tmax - tmin) = 2000, then calib size is 2001 !
    const int idxUnder = (int)floor(idx);
    const int idxUpper = (int)ceil(idx);//idxUnder + 1;

    if (idxUnder == idxUpper) {
        return curve[idxUnder];

    } else if (curve[idxUnder] != 0. && curve[idxUpper] != 0.) {
        // Important for gate: no interpolation around gates
        return interpolate((double) idx, (double)idxUnder, (double)idxUpper, curve[idxUnder], curve[idxUpper]);

    } else {
        return 0.;
    }

}

/**
    @brief  This function make a QMap which are a copy of the QMap aMap to obtain an percent of area
    @brief  to define a area we need at least 2 value in the map
    @param threshold is in percent
 */


const std::map<double, double> create_HPD(const QMap<double, double>& aMap, const double threshold)
{
    std::map<double, double> result;

    if (aMap.size() < 2)  // in case of only one value (e.g. a bound fixed) or no value
        return result;


    const long double areaTot = map_area(aMap);

    std::map<double, double> aMapStd = aMap.toStdMap();
    if (areaTot==threshold) {
        result = aMapStd;
        return result;

    } else {
        try {
            std::multimap<double, double> inverted;
            std::multimap<double, double>::const_iterator cIter = aMapStd.cbegin();

            while (cIter != aMapStd.cend()) {
                 const double t = cIter->first;//key();
                 const double v = cIter->second;//value();
                 result[t] = 0.; // important to init all the possible value

                inverted.insert(std::pair<double, double>(v, t));
                 ++cIter;
            }

            typedef std::multimap<double, double>::iterator iter_type;
            std::reverse_iterator<iter_type> iterInverted (inverted.rbegin());


            long double area (0.);
            long double areaSearched = areaTot * threshold / 100.;

            iterInverted++;
        //--------------------
            while (std::next(iterInverted) != inverted.rend()) {
                iterInverted++;
                double t = iterInverted->second;//.value();
                const double v = iterInverted->first;//.key();

                std::map<double, double> ::iterator iterMap = aMapStd.find(t);

                //    This part of code fix the case of irregular QMap when the step between keys are not the same

                if (iterMap->first == t) { // meaning : find(t) find the good key else iterMap = end()

                    if ( iterMap != aMapStd.begin() && iterMap != aMapStd.end()) { // meaning : iterMap is not the first item

                        const double vPrev = std::prev(iterMap)->second;
                        if (vPrev >= v ) {
                            const double tPrev = std::prev(iterMap)->first;
                            area += (long double)(v + vPrev)/2 * (long double)(t - tPrev);
                            // we need to save a surface, so we need to save 4 values
                            if (area < areaSearched) {
                                result[t] = v;
                                result[tPrev] = vPrev;
                            }
                        }
                    }

                    if (iterMap != aMapStd.end() ) {
                        const double vNext = std::next(iterMap)->second;
                        if (vNext > v) {
                            const double tNext = std::next(iterMap)->first;

                            area += (long double)(v + vNext)/2. * (long double)(tNext - t);
                            // we need to save a surface, so we need to save 4 values
                            if (area < areaSearched) {
                                result[t] = v;
                                result[tNext] = vNext;
                            }

                        }
                    }

                }

                if (std::next(iterInverted) != inverted.rend() &&  (std::prev(iterInverted)->first==v) ) {
                     result[t] = v;

                } else {
                        if (area < areaSearched)
                            result[t] = v;

                        else if (area >= areaSearched)
                            return result;

                }
            }

            return result;
       }
       catch (std::exception const & e) {
            qDebug()<< "in stdUtilities::create_HPD() Error"<<e.what();
            return aMapStd;
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
    ++cIter;
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
            histo[int (idx_under)] += contrib_under;

        if (idx_upper < nbPts) // This is to handle the case when matching the last point index !
            histo[int (idx_upper)] += contrib_upper;
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
