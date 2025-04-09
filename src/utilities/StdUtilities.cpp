/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2024

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

#include <QtGlobal>

#include <ctgmath>
#include <cstdlib>
#include <fenv.h>
#include <QObject>
#include <thread>
#include <valarray>
#include <iterator>

using namespace std;

// unused
int compareStrings(const string &s1, const string &s2)
{
    const size_t m = s1.size();
    const size_t n = s2.size();
    // If one of the strings is empty, return the length of the other string
    if (m == 0) return n;
    if (n == 0) return m;

    std::vector<int> prev(n + 1);
    std::vector<int> crt(n + 1);

    for (size_t j = 0; j <= n; ++j) {
        prev[j] = j; // Distance from empty string to s2
    }

    for (size_t i = 1; i <= m; i ++) {
        crt[0] = i; // par construction il semble inutile de procéder au moindre reset sur les autres colonnes
        for (size_t j = 1; j <= n; j++) {
            int cost = (s1[i-1] == s2[i-1]) ? 0 : 1;
            crt[j] = std::min({prev[j] + 1, crt[j - 1] + 1, prev[j - 1] + cost});
        }
        std::swap( prev, crt);
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



double safeExp(const double x, int n)
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

double safeLog(const double x, int n)
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
 @brief This function transforms a QList turning its maximum value is 1 and adjusting other values accordingly
 **/
QList<double> normalize_vector(const QList<double> &vector)
{
    QList<double> histo;

    QList<double>::const_iterator it = max_element(vector.begin(), vector.end());
    if (it != vector.end()) {
        const double max_value = *it;

        for (const auto& value : vector)
            histo.push_back(value/max_value);
    }
    return histo;
}

QList<float> normalize_vector(const QList<float>& vector)
{
    QList<float> histo;

    QList<float>::const_iterator it = max_element(vector.begin(), vector.end());
    if (it != vector.end()){
        float max_value = *it;
        for (QList<float>::const_iterator it = vector.begin(); it != vector.end(); ++it)
            histo.push_back((*it)/max_value);

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




QMap<double, double> equal_areas(const QMap<double, double> &mapToModify, const double targetArea)
{
    if (mapToModify.isEmpty())
        return QMap<double, double>();

    QMap<double, double> result;
    const double srcArea = map_area(mapToModify);

    const double prop = targetArea / srcArea;

    for (auto it = mapToModify.begin(); it != mapToModify.end(); ++it)
    {
        result.insert(it.key(), it.value()*prop);
    }


   
   /* auto keyVal_range = mapToModify.asKeyValueRange(); //https://doc.qt.io/qt-6/containers.html#implicit-sharing-iterator-problem
    for (auto [key, value] : keyVal_range) {
        //value *= prop;
        const double tmp = value*prop;
        
        result.insert(key, tmp);
    }*/
    
    return result;
}

QMap<float, float> equal_areas(const QMap<float, float> &mapToModify, const float targetArea)
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

std::map<double, double> equal_areas(const std::map<double, double> &mapToModify, const double targetArea)
{
    if (mapToModify.empty())
        return std::map<double, double>();

    std::map<double, double> result;
    const double srcArea = map_area(mapToModify);

    const double prop = targetArea / srcArea;

    for (auto it = mapToModify.begin(); it != mapToModify.end(); ++it)
    {
        result.emplace(it->first, it->second *prop);
    }

    return result;
}

std::map<double, double> equal_areas(const std::map<double, double>& mapToModify, const std::map<double, double>& mapWithTargetArea)
{
    if (mapToModify.empty())
        return std::map<double, double>();

    std::map<double, double>::const_iterator iter(mapWithTargetArea.begin());
    double targetArea = 0.;
    while (iter != std::prev(mapWithTargetArea.end())) {
        std::advance(iter, 1);
        targetArea += iter->second;
    }
    return equal_areas(mapToModify, targetArea);
}

QList<double> equal_areas(const QList<double> &data, const double step, const double area)
{
    if(data.isEmpty())
        return QList<double>();

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
    QList<double> result;

    QList<double>::const_iterator cIter = data.cbegin();
    while (cIter != data.cend() ) {
        result.append(*cIter / invProp);
        ++cIter;
    }

    return result;
}

QList<float> equal_areas(const QList<float>& data, const float step, const float area)
{
    if (data.isEmpty())
        return QList<float>();

    long double srcArea (0.l);
    long double lastV = data.at(0);

    for (auto&& value : data) {
        const long double v = value;

        if (lastV>0.l && v>0.l)
            srcArea += (lastV+v)/2.l * (long double)step;

       lastV = v;
    }

    const long double invProp =srcArea / area;
    QList<float> result;

    QList<float>::const_iterator cIter = data.cbegin();
    while (cIter != data.cend() ) {
        result.append(float (*cIter / invProp));
        ++cIter;
    }

    return result;
}

std::vector<double> equal_areas(const std::vector<double>& data, const float step, const float area)
{
    if (data.empty())
        return {};

    long double srcArea (0.l);
    long double lastV = data.at(0);

    for (auto&& value : data) {
        const long double v = value;

        if (lastV>0.l && v>0.l)
            srcArea += (lastV+v)/2.l * (long double)step;

        lastV = v;
    }

    const long double invProp =srcArea / area;
    std::vector<double> result;

    std::vector<double>::const_iterator cIter = data.cbegin();
    while (cIter != data.cend() ) {
        result.push_back(double (*cIter / invProp));
        ++cIter;
    }

    return result;
}

QMap<double, double> vector_to_map(const QList<double>& data, const double min, const double max, const double step)
{
    Q_ASSERT(max>=min);

    QMap<double, double> map;

    if (data.isEmpty())
        return {};

    if (min == max)
        map.insert(min, data.at(0));

    else {
        map.insert(min, data.at(0));
        
        const auto nbPts = data.size() - 1;//int ((max - min) / step);

        for (qsizetype i = 1; i< nbPts; ++i) {
            map.insert(min + i * step, data.at(i));
        }
        map.insert(max, data.last());
    }

    return map;
}

QMap<double, double> vector_to_map(const QList<int>& data, const double min, const double max, const double step)
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


std::map<double, double> vector_to_map(const std::vector<double>& data, const double min, const double max, const double step)
{
    //Q_ASSERT(max>=min && !data.empty());
    std::map<double, double> map ;
    if (data.empty())
        return map;

    if (min == max) {
        if (data.empty()){
            return {};
        } else {
            map.emplace(min, data.at(0));
        }

    } else {
        const size_t nbPts = std::min (1 + (size_t)round((max - min) / step), data.size()); // step is not usefull, it's must be data.size/(max-min+1)

        for (size_t i = 0; i<nbPts; ++i) {
            auto t = min + i * step;
            map.emplace(t,  double (data.at(i)));

        }
    }
    return map;
}

QMap<float, float> vector_to_map(const QList<float>& data, const float min, const float max, const float step)
{
    Q_ASSERT(max>=min && !data.isEmpty());
    QMap<float, float> map;
    if (min == max) {
        if (data.empty()){
            return {};
        } else {
            map.insert(min, data.at(0));
        }

    } else {
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
 * @brief interpolate_value_from_curve Allows you to find the value associated with a time in a curve.
 * @param t Time for which we are looking for value
 * @param curve
 * @param curveTmin Time corresponding to index 0
 * @param curveTmax Time corresponding to index curve.size()-1
 * @return
 */
double interpolate_value_from_curve(const double t, const QList<double> &curve,const double curveTmin, const double curveTmax)
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
        return interpolate( idx, (double)idxUnder, (double)idxUpper, curve[idxUnder], curve[idxUpper]);

    } else {
        return 0.;
    }

}

double interpolate_value_from_curve(const double x, const std::vector<double>& curve, const double Xmin, const double Xmax)
{
     // We need at least two points to interpolate
    if (curve.size() < 2 || x <= Xmin) {
        return curve.front();

    } else if (x >= Xmax) {
        return curve.back();
    }

    const double prop = (x - Xmin) / (Xmax - Xmin);
    const double px = prop * (curve.size() - 1); // tricky : if (Xmax - Xmin) = 2000, then calib size is 2001 !
    const double idxUnder = floor(px);
    const double idxUpper = ceil(px);//idxUnder + 1;

    if (idxUnder == idxUpper) {
        return curve[idxUnder];

   // } else if (curve[idxUnder] != 0. && curve[idxUpper] != 0.) {
    } else if (curve[idxUpper] != 0.) {
        // Important for gate: no interpolation around gates
        return interpolate( px, idxUnder, idxUpper, curve[(int)idxUnder], curve[(int)idxUpper]);

    } else {
        return 0.;
    }

}

// Ne teste pas la limite debut fin density, on supose un pas régulier, utilisé avec create_HPD2()
double surface_on_theta (std::map<double, double>::const_iterator iter_on_theta)
{
    const auto &prev_iter = std::prev(iter_on_theta);
    const auto &next_iter = std::next(iter_on_theta);
    double S1 = 0.0, S2 = 0.0;
    if (prev_iter->second >0)
        S1 = (3.*iter_on_theta->second + prev_iter->second) * (iter_on_theta->first - prev_iter->first) / 8.;

    if (next_iter->second >0)
        S2 = (3.*iter_on_theta->second + next_iter->second) * (next_iter->first - iter_on_theta->first) /8.;

    return S1+S2;

}

/**
 * @brief create_HPD2
 * @param density
 * @param threshold en percent
 * @return
 */
const std::map<double, double> create_HPD2(const QMap<double, double> &density, const double threshold)
{
    std::map<double, double> result;

    if (density.size() < 2) { // in case of only one value (e.g. a bound fixed) or no value
        if (density.size() < 1) { // in case of  no value
            return result;
        }
        result[density.firstKey()] = 1.;
        return result;
    }

    const double areaTot = map_area(density);

    std::map<double, double> mapStd = density.toStdMap();
    if (areaTot == threshold/100.) {
        result = mapStd;
        return result;

    } else {
        try {
            std::multimap<double, std::map<double, double>::const_iterator> inverted;

            for (std::map<double, double>::const_iterator m = mapStd.cbegin(); m!= mapStd.cend(); m++) {
                const double v = m->second;
                result[m->first] = 0;
                inverted.insert({v, m});
            }

            const double areaSearched = areaTot * threshold / 100.;

            std::multimap<double, std::map<double, double>::const_iterator> ::const_reverse_iterator riter = inverted.crbegin();

            const auto iter_on_last_mapStd_value = std::prev(mapStd.cend());
            double area = 0.;
            while (area < areaSearched && riter!= inverted.crend()) {
               // qDebug()<<"create_HPD rentre seuil "<<riter->first;

                if (riter->second != iter_on_last_mapStd_value && riter->second != mapStd.cbegin()) {
                    area += surface_on_theta(riter->second);
                   // qDebug()<<"riter theta"<<riter->second->first<<" seuil"<<riter->first<<area;

                } else if (riter->second != iter_on_last_mapStd_value) { // donc c'est le debut
                    const auto next_iter = std::next(riter->second);
                    area += (3.*riter->first + next_iter->second) * (next_iter->first - riter->second->first) /8.;

                } else { // c'est donc la fin
                    const auto prev_iter = std::prev(riter->second);
                    area += (3.*riter->first + prev_iter->second) * (riter->second->first - prev_iter->first) / 8.;
                }
                if (area > areaSearched)
                    break;

                //qDebug()<<"iter"<<riter->first<<area;
                ++riter;
            }

            if (riter == inverted.crend())
                --riter;

            const double threshSearched = riter->first;

            // ------------------Creation de result

            std::map<double, double> ::iterator iterMap = mapStd.begin();
            while (iterMap != mapStd.end()) {
                const double t = iterMap->first;
                const double v = iterMap->second;

                if (v >= threshSearched)
                    result[t] = v;
                else
                    result[t] = 0;

                iterMap++;
            }
            // test si result est vide, signifie qu'on a une valeur constante
            if (result.empty()) {
                const double t = mapStd.begin()->first;
                const double v = mapStd.begin()->second;
                result[t]= v;

                qDebug()<< "[stdUtilities::create_HPD2] one solution for "<< t;
            }
            return result;

        }
        catch (std::exception const & e) {
            qDebug()<< "[stdUtilities::create_HPD2] Error " << e.what();
            return mapStd;
        }

    }
}

/**
 * @brief create_HPD_mapping, determines the part of the curve that corresponds to the requested threshold and creates a area_mapping that maps each date to its participation in the sum under the density
 * @param density
 * @param area_mapping
 * @param threshold
 * @return a copy of the density corresponding to the requested threshold. The rest of the density is set to zero.
 */
const std::map<double, double> create_HPD_mapping(const QMap<double, double> &density, std::map<double, double> &area_mapping, const double threshold)
{
    std::map<double, double> result;

    if (density.size() < 2) { // in case of only one value (e.g. a bound fixed) or no value
        if (density.size() < 1) { // in case of  no value
            return result;
        }
        result[density.firstKey()] = 1.;
        return result;
    }

    const double areaTot = map_area(density);

    std::map<double, double> mapStd = density.toStdMap();
    if (areaTot == threshold/100.) {
        result = mapStd;
        return result;

    } else {
        try {
            std::multimap<double, std::map<double, double>::const_iterator> inverted;

            for (std::map<double, double>::const_iterator m = mapStd.cbegin(); m!= mapStd.cend(); m++) {
                const double v = m->second;
                result[m->first] = 0;
                inverted.insert({v, m});
            }

            const double areaSearched = areaTot * threshold / 100.;

            std::multimap<double, std::map<double, double>::const_iterator> ::const_reverse_iterator riter = inverted.crbegin();

            const auto iter_on_last_mapStd_value = std::prev(mapStd.cend());
            double area = 0., tmp_area;
            double threshSearched = 2.;//

            while (riter!= inverted.crend()) {

                const double t = riter->second->first;
                const double v = riter->second->second;
                if (area > areaSearched && v < threshSearched ) {
                    area_mapping.insert({t, 0.});


                } else {
                    if (riter->second != iter_on_last_mapStd_value && riter->second != mapStd.cbegin()) {
                        tmp_area = surface_on_theta(riter->second);


                    } else if (riter->second != iter_on_last_mapStd_value) { // donc c'est le debut
                        const auto next_iter = std::next(riter->second);
                        tmp_area = (3.*riter->first + next_iter->second) * (next_iter->first - riter->second->first) /8.;

                    } else { // c'est donc la fin
                        const auto prev_iter = std::prev(riter->second);
                        tmp_area = (3.*riter->first + prev_iter->second) * (riter->second->first - prev_iter->first) / 8.;
                    }
                    area += tmp_area;
                    area_mapping.insert({t, tmp_area});
                    result[t] = v;
                    threshSearched = v;
                }

                ++riter;
            }



            // test si result est vide, signifie qu'on a une valeur constante
            if (result.empty()) {
                const double t = mapStd.begin()->first;
                const double v = mapStd.begin()->second;
                result[t]= v;
                area_mapping.insert({t, 1.});
                qDebug()<< "[stdUtilities::create_HPD_mapping] one solution for "<< t;
            }
            return result;

        }
        catch (std::exception const & e) {
            qDebug()<< "[stdUtilities::create_HPD_mapping] Error " << e.what();
            return mapStd;
        }

    }
}

const std::map<double, double> create_HPD_by_dichotomy(const QMap<double, double> &density, QList<QPair<double, QPair<double, double> > > &intervals_hpd, const double threshold)
{
    int nb_max_loop = 20;
    std::map<double, double> result;

    if (density.size() < 2) { // in case of only one value (e.g. a bound fixed) or no value
        if (density.size() < 1) { // in case of  no value
            intervals_hpd = QList<QPair<double, QPair<double, double> > >();
            return result;
        }
        intervals_hpd.append({1., QPair<double, double>(density.firstKey(), density.firstKey())});
        result[density.firstKey()] = 1.;
        return result;
    }

    const double area_tot = map_area(density);

    std::map<double, double> mapStd = density.toStdMap();
    if (area_tot == threshold/100.) {  // ???
        intervals_hpd.append({1., QPair<double, double>(density.firstKey(), density.lastKey())});
        result = mapStd;
        return result;

    } else {
        std::map<double, double> tmp_hpd;

        auto Vmax = std::max_element(mapStd.begin(), mapStd.end(), [](const auto &p1, const auto &p2) {return p1.second < p2.second; });

        double v_sup = Vmax->second;
        double v_inf = 0;
        double v = Vmax->second;
        const double area_target =  area_tot * threshold/100.;
        double area_loop = 0., area_inter;
        bool inter_open = false;

        for (int n = 0 ; n < nb_max_loop; n++) {
            area_loop = 0.;
            intervals_hpd.clear();
            QPair<double, double> inter;
            inter_open = false;
            area_inter = 0;

            v = (v_sup + v_inf)/2.;

            double t_prev = mapStd.begin()->first;
            double v_prev = mapStd.begin()->second;


            for (const auto &d : mapStd) {

                if (v_prev <v && d.second >= v && v_prev>0.) {
                    const double t = interpolate(v, v_prev, d.second, t_prev, d.first);
                    const auto S = (v + d.second) * (d.first - t)/2.;
                    area_loop += S;
                    area_inter = S;

                    inter.first = t;
                    inter_open = true;

                } else if (v_prev >=v && d.second >= v) {
                    if (inter_open == false) {
                        inter.first = t_prev;
                        inter_open = true;
                    }
                    area_loop += (v_prev + d.second)* (d.first - t_prev) / 2.;
                    area_inter += (v_prev + d.second)* (d.first - t_prev) / 2.;

                } else if (v_prev >= v && d.second <= v && d.second > 0.) {

                    const double t =  interpolate(v, v_prev, d.second, t_prev, d.first);
                    const auto S = (v_prev + v) * (t - t_prev)/2.;
                    area_loop += S;
                    area_inter += S;

                    inter.second = t;
                    inter_open = false;

                    intervals_hpd.append({area_inter, inter});
                }

                t_prev = d.first;
                v_prev = d.second;
            }
            if (inter_open == true) {
                inter.second = t_prev;
                intervals_hpd.append({area_inter, inter});
            }
            if (area_loop > area_target) {
                v_inf = v;

            } else {
                v_sup = v;
            }

        }

        for (auto&& d : mapStd) {
            if (d.second < v)
                d.second = 0.;
        }

        return mapStd;
    }


}

const std::map<double, double> create_HPD_by_dichotomy(const std::map<double, double> &density, QList<QPair<double, QPair<double, double> > > &intervals_hpd, const double threshold)
{
    // Gestion des cas spéciaux
    if (density.size() < 2) {
        intervals_hpd.clear();
        if (density.empty()) {
            return {};
        }

        const auto& first_point = *density.begin();
        intervals_hpd.append({1.0, {first_point.first, first_point.first}});
        return {{first_point.first, 1.0}};
    }
    //

    // Initialisation de la recherche dichotomique
    auto max_density_it = std::max_element(
        density.begin(), density.end(),
        [](const auto& p1, const auto& p2) { return p1.second < p2.second; }
        );

    double v_sup = max_density_it->second;
    double v_inf = 0.0;

    // Copie modifiable de la densité
    std::map<double, double> result_density = density;



//
    // Calcul de l'aire totale et de l'aire cible
    const double area_tot = map_area(density);
    const double area_target = area_tot * threshold / 100.0;

    // Cas limite : aire totale égale à l'aire cible
    if (std::abs(area_tot - area_target) < std::numeric_limits<double>::epsilon()) {
        intervals_hpd.append({1.0, {density.begin()->first, density.rbegin()->first}});
        return density;
    }

    QList<QPair<double, QPair<double, double>>> final_intervals;
    //double final_total_area = 0.0;
    double final_current_threshold = 0.0;
    //double area_total_since_prev_interval = 0;
    // Recherche dichotomique
    const int max_iterations = 20;
    for (int iteration = 0; iteration < max_iterations; ++iteration) {
        double current_threshold = (v_sup + v_inf) / 2.0;

        // Calcul des intervalles HPD
        QList<QPair<double, QPair<double, double>>> current_intervals;
        double total_area = 0.0;
        double area_total_since_prev_interval = 0.0;

        auto prev_it = density.begin();
        bool interval_open = (prev_it->second < current_threshold);//false;
        QPair<double, double> current_interval;

        if (interval_open){
            double t = density.begin()->first;
            current_interval.first = t;
        }
        for (auto it = std::next(density.begin()); it != density.end(); ++it) {
            // Début d'un intervalle
            if (prev_it->second < current_threshold && it->second >= current_threshold) {
                double t = interpolate(current_threshold, prev_it->second, it->second,
                                       prev_it->first, it->first);
                current_interval.first = t;
                interval_open = true;
            }

            // Intérieur d'un intervalle
            if (it->second >= current_threshold) {
                if (!interval_open) {
                    current_interval.first = prev_it->first;
                    interval_open = true;
                }

                // Calcul de l'aire partielle
                double area = (prev_it->second + it->second) * (it->first - prev_it->first) / 2.0;
                total_area += area;
            }

            // Fin d'un intervalle, dans le cas d'une valeur unique supérieur au seuil au debut de l'intervalle, il n'y a pas
            if (interval_open && prev_it->second >= current_threshold && it->second < current_threshold) {
                double t = interpolate(current_threshold, prev_it->second, it->second,
                                       prev_it->first, it->first);
                current_interval.second = t;
                current_intervals.append({total_area - area_total_since_prev_interval, current_interval});
                area_total_since_prev_interval = total_area;
                interval_open = false;
            }

            prev_it = it;
        }

        // Gestion du dernier intervalle potentiel
        if (interval_open) {
            current_interval.second = prev_it->first;
            current_intervals.append({total_area - area_total_since_prev_interval, current_interval});
            area_total_since_prev_interval = total_area;
        }

        // Ajustement de la recherche dichotomique
        if (total_area > area_target) {
            v_inf = current_threshold;
        } else {
            v_sup = current_threshold;
        }

        // Convergence suffisante
        if (std::abs(total_area - area_target) < area_target * 0.001) {
            intervals_hpd = current_intervals;

            // Mise à zéro des densités inférieures au seuil
            for (auto& point : result_density) {
                if (point.second < current_threshold) {
                    point.second = 0.0;
                }
            }

            return result_density;
        }
        // Stocker les derniers résultats à chaque itération
        final_intervals = current_intervals;
        //final_total_area = total_area;
        final_current_threshold = current_threshold;
    }

    // Fallback après max_iterations
    intervals_hpd = final_intervals;

    // Mise à zéro des densités inférieures au seuil
    for (auto& point : result_density) {
        if (point.second < final_current_threshold) {
            point.second = 0.0;
        }
    }

    return result_density;

    // -old
 /*
    int nb_max_loop = 20;
    std::map<double, double> result;

    std::map<double, double> mapStd = density;
    if (area_tot == threshold/100.) {  // ???
        intervals_hpd.append({1., QPair<double, double>(density.begin()->first, density.crbegin()->first)});
        result = mapStd;
        return result;

    } else {
        std::map<double, double> tmp_hpd;

        auto Vmax = std::max_element(mapStd.begin(), mapStd.end(), [](const auto &p1, const auto &p2) {return p1.second < p2.second; });

        double v_sup = Vmax->second;
        double v_inf = 0;
        double v = Vmax->second;
        const double area_target =  area_tot * threshold/100.;
        double area_loop = 0., area_inter;
        bool inter_open = false;

        for (int n = 0 ; n < nb_max_loop; n++) {
            area_loop = 0.;
            intervals_hpd.clear();
            QPair<double, double> inter;
            inter_open = false;
            area_inter = 0;

            v = (v_sup + v_inf)/2.;

            double t_prev = mapStd.begin()->first;
            double v_prev = mapStd.begin()->second;


            for (const auto &d : mapStd) {

                if (v_prev <v && d.second >= v && v_prev>0.) {
                    const double t = interpolate(v, v_prev, d.second, t_prev, d.first);
                    const auto S = (v + d.second) * (d.first - t)/2.;
                    area_loop += S;
                    area_inter = S;

                    inter.first = t;
                    inter_open = true;

                } else if (v_prev >=v && d.second >= v) {
                    if (inter_open == false) {
                        inter.first = t_prev;
                        inter_open = true;
                    }
                    area_loop += (v_prev + d.second)* (d.first - t_prev) / 2.;
                    area_inter += (v_prev + d.second)* (d.first - t_prev) / 2.;

                } else if (v_prev >= v && d.second <= v && d.second > 0.) {

                    const double t =  interpolate(v, v_prev, d.second, t_prev, d.first);
                    const auto S = (v_prev + v) * (t - t_prev)/2.;
                    area_loop += S;
                    area_inter += S;

                    inter.second = t;
                    inter_open = false;

                    intervals_hpd.append({area_inter, inter});
                }

                t_prev = d.first;
                v_prev = d.second;
            }
            if (inter_open == true) {
                inter.second = t_prev;
                intervals_hpd.append({area_inter, inter});
            }
            if (area_loop > area_target) {
                v_inf = v;

            } else {
                v_sup = v;
            }

        }

        for (auto&& d : mapStd) {
            if (d.second < v)
                d.second = 0.;
        }

        return mapStd;
    }
*/

}



double map_area(const QMap<double, double> &map)
{
    return map_area(map.toStdMap());
}

float map_area(const QMap<float, float> &map)
{
    if (map.size() < 2)
        return 0.f;

    QMap<float, float>::const_iterator iter = map.cbegin();
    float area = 0.f;

    float lastV = iter.value();
    float lastT = iter.key();

    while (iter != map.cend()) {
        const float v = iter.value();
        const float t = iter.key();
        if (lastV>0 && v>0)
            area += (lastV+v)/2.f * (t-lastT);


        lastV = v;
        lastT = t;
        ++iter;
    }

    return area;
}

double map_area(const std::map<double, double> &map)
{
    if (map.size() < 1)
        return 0.0;
    else if (map.size() == 1)
        return 1.;

    std::map<double, double>::const_iterator iter = map.cbegin();
    double area = 0.;

    double lastV = iter->second;
    double lastT = iter->first;
    ++iter;
    while (iter != map.cend())  {
        const double v = iter->second;
        const double t = iter->first;
        if (lastV>0 && v>0) {
            area += (lastV+v)/2. * (t-lastT);

        } /*else if (lastV==0 && v>0) {
            area += 3.*v/8. * (t-lastT);

        } else if (lastV>0 && v==0) {
            area += 3.*lastV/8. * (t-lastT);
        }*/

        lastV = v;
        lastT = t;

        ++iter;
    }

    return area;
}


double map_area(const QMap<int, double> &density)
{
    //return std::accumulate(density.constBegin(), density.constEnd(), 0., [](double sum, auto m){return sum + m;  });
    return std::accumulate(density.constBegin(), density.constEnd(), 0., std::plus<double>());
}

QList<double> vector_to_histo(const QList<double> &vector, const double tmin, const double tmax, const int nbPts)
{
    QList<double> histo;
    //histo.reserve(nbPts);
    histo.fill(0., nbPts);
    const double delta = (tmax - tmin) / (nbPts - 1);

    const double denum = vector.size();

    for (QList<double>::const_iterator iter = vector.cbegin(); iter != vector.cend(); ++iter) {
        const double t = *iter;

        const double idx = (t - tmin) / delta;
        const double idx_under = floor(idx);
        const double idx_upper = idx_under + 1.;

        const double contrib_under = (idx_upper - idx) / denum;
        const double contrib_upper = (idx - idx_under) / denum;
#ifdef DEBUG
        if (std::isinf(contrib_under) || std::isinf(contrib_upper))
            qDebug() << "StdUtilities::vector_to_histo() : infinity contrib!";

        if (idx_under < 0 || idx_under >= nbPts || idx_upper < 0 || idx_upper > nbPts)
            qDebug() << "StdUtilities::vector_to_histo() : Wrong index";
#endif

        if (idx_under < nbPts)
            histo[int (idx_under)] += contrib_under;

        if (idx_upper < nbPts) // This is to handle the case when matching the last point index !
            histo[int (idx_upper)] += contrib_upper;
    }

    bool bEmpty = true;
    for (QList<double>::const_iterator iter = histo.cbegin();( iter != histo.cend())&& bEmpty; ++iter) {
         if (*iter> 0)
             bEmpty= false;
    }
    if (bEmpty)
        qDebug()<<"in vector_to_histo histo is empty !!!!";

    return histo;
}

/**
 * @brief polynom_filter
 * @param data value of X and Y
 * @param d  degree polynomial
 * @return d+1 coef of polynom
 */
std::valarray<double> polynom_regression_coef(QMap<double, double> &data,  int d)
{

    std::valarray<std::valarray<double>> c (std::valarray<double>(d+2), d+1);
    for (int j=0; j<d+1; j++)
        for (int k=0; k<d+1; k++) {
             c[j][k] = 0;

            for (auto [key, value] : data.asKeyValueRange()) {
                c[j][k] += pow(key, j+k);
            }

        }
    for (int j=0; j<d+1; j++) {
        c[j][d+1] = 0.;

        for (auto [key, value] : data.asKeyValueRange()) {
             c[j][d+1] += value*pow(key, j);;
        }
    }

    for (int k=0; k<d+1; k++)
        for (int i=0; i<d+1; i++) {
             if (i!=k) {
                const double u = c[i][k]/c[k][k];
                for (int j=k; j<d+2; j++) {
                    c[i][j] -= u * c[k][j];
                }
             }
        }

    std::valarray<double> a(d+1);
    for (int i=0; i<d+1; i++) {
        a[i] = c[i][d+1]/c[i][i];
       // qDebug()<<"[polynom_regresssion_coef] i="<< i<<" a="<< a[i];
    }

    return a;
}

/**
 * @brief MSE Mean Squared Error
 * @param data
 * @param polynom_coef
 * @return
 */
double MSE(const QMap<double, double> &data,  const std::valarray<double> polynom_coef)
{
    double mse = 0;
    for (auto [key, value] : data.asKeyValueRange()) {
        double f = 0;
        for (int i = 0 ; i< (int)polynom_coef.size() ; i++)
            f += polynom_coef[i] * pow(key, i);
        mse += pow(value - f, 2.);
    }
    return mse/data.size();
}




/**
 Pour qUnif, détermine la courbe x= r (q)
 On suppose q variant de 0 à 1 avec un pas de 1/q_frac
 retourne un tableau de taille q_frac+1 et x est une fréquence entre 0 et 1
 */
//binomialeCurveByLog testé avec n=5000
std::vector<double> binomialeCurveByLog(const int n, const double alpha, const int q_frac)
{

    std::vector<double> Rq;

    if (n<2) {
        Rq = std::vector<double>(q_frac, 1.);
        return Rq;
    }

    const double alpha2 = alpha/2.;

    double q, lnP = 0, prev_sum = 0, sum_p = 0.;

    double x, R, last_x = 1., last_q = 0.;


    // p = 0%
    Rq.push_back(0);

    for (int i = 1; i< q_frac; ++i) {
        q = i/(double) q_frac;

        R = log(q/(1-q)); // La raison de la suite
        x = 0;
        // k=0
        lnP = n*log(1-q);
        sum_p = exp(lnP);
        prev_sum = sum_p;
        if (sum_p >= alpha2) {
            Rq.push_back(x/n);
            continue;

        } else {
            x = -1;
            for (int k = 1; k<n && x == -1; ++k) {

               lnP += (R+ log((double)(n-k+1)/(double)k) );
               sum_p += exp(lnP);

                if (sum_p == alpha2) {
                   x = k;

               } else  if (prev_sum < alpha2 && alpha2< sum_p) {
                   const auto proba = exp(lnP);
                   x = (alpha2-prev_sum)/proba + k -1;

               }

               prev_sum = sum_p;

            }
            if (x == -1) {
                x = interpolate( q , last_q, 1., last_x, (double) n);

            } else {
                last_x = x;
                last_q = q;
            }

            Rq.push_back(x/n);
        }

    }
    // p = 100%
    Rq.push_back(n/n);

    return Rq;

}

/**
 Pour qActivity, détermine la courbe p = g (x)  en inversant x = r (p) ; p une fréquence entre 0 et 1
 taille x_frac+1
 */
std::vector<double> inverseCurve(const std::vector<double> &Rq, const int x_frac)
{
    double x, p;
    const double p_frac = Rq.size() - 1.;
    double j;
    std::vector<double> Gx;
    for (int i = 0; i<= x_frac; ++i) {
        x = (double)(i) / x_frac;
        p = 0.;
        j = 0;
        while ( j<Rq.size() && Rq[j] <= x) {
            p = j / p_frac;
            ++j;
        }
        if (Rq[j-1] == x)
            Gx.push_back(p);
        else
            Gx.push_back( interpolate( x, Rq[j-1], Rq[j], p, p + 1./p_frac));

    }
    return Gx;
}


double findOnOppositeCurve (const double x, const std::vector<double> &Gx)
{
    const double x_frac = Gx.size() - 1;
    const double one_X = (1 - x)*x_frac;
    return 1. - interpolate_value_from_curve( one_X, Gx, 0, x_frac);

}



#pragma mark Chronometer
Chronometer::Chronometer():
    _comment ("Chronometer"),
    _start (std::chrono::high_resolution_clock::now())
{
}

Chronometer::Chronometer(std::string comment):
    _comment (comment),
    _start (std::chrono::high_resolution_clock::now())
{
}

Chronometer::~Chronometer()
{
}

void Chronometer::display()
{
    auto d = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - _start);

    std::thread th_display ([] (std::string _comment, std::chrono::duration<long long, micro> du) {
        if ( du == std::chrono::steady_clock::duration::zero() ) {
            qWarning() << " " << QString().fromStdString(_comment) << " -> The internal clock did not tick.\n";

        } else {
            qWarning()<< "chrono "<< QString().fromStdString(_comment) <<" " << du.count() << " µs \n";

        }
    }, _comment, d);
    th_display.detach();

}

std::chrono::microseconds Chronometer::eval()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - _start);
}
