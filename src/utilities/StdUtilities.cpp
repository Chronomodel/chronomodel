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
#include "Functions.h"

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
        map.emplace(min, data.front());
        return map;
    }
    // Nombre de points attendu (gridLength)
    const size_t expectedPts = data.size();

    // On évite les problèmes de round : on force nbPts = data.size()
    for (size_t i = 0; i < expectedPts; ++i) {
        double t = min + i * step;
        if (t > max + 1e-9) // tolérance pour éviter de dépasser max
            break;
        map.emplace(t, data[i]);
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


/* old code
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
    const double idxUnder = floor(idx);
    const double idxUpper = ceil(idx);//idxUnder + 1;

    if (idxUnder == idxUpper) {
        return curve[idxUnder];

    } else if (curve[idxUnder] != 0. && curve[idxUpper] != 0.) {
        // Important for gate: no interpolation around gates
        return interpolate( idx, idxUnder, idxUpper, curve[(int)idxUnder], curve[(int)idxUpper]);

    } else {
        return 0.0;
    }

}*/



/*double interpolate_value_from_curve(const double x, const std::vector<double>& curve, const double Xmin, const double Xmax)
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

}*/



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

/*
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


}*/

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


double erfcInv_approx(double x)
{
    if (x <= 0.0) return std::numeric_limits<double>::max();  // +∞
    if (x >= 2.0) return -std::numeric_limits<double>::max(); // -∞

    // Transforme erfc⁻¹ en Φ⁻¹ (inverse de la loi normale standard)
    //   Φ⁻¹(p) = √2 * erfc⁻¹(2(1‑p))
    // Nous allons donc d’abord calculer Φ⁻¹(p) puis revenir à erfc⁻¹.
    // Mais il est plus simple de travailler directement sur Φ⁻¹.

    // p = 1 - x/2  ∈ (0,1)
    double p = 1.0 - 0.5 * x;

    // ---- 1. Approximation de Φ⁻¹(p) ----
    // Si p < 0.5 on utilise la symétrie Φ⁻¹(p) = -Φ⁻¹(1‑p)
    bool neg = false;
    if (p < 0.5) {
        p = 1.0 - p;
        neg = true;
    }

    // Coefficients de la série (Abramowitz & Stegun, 26.2.23)
    const double a0 =  2.50662823884;
    const double a1 = -18.61500062529;
    const double a2 =  41.39119773534;
    const double a3 = -25.44106049637;

    const double b0 = -8.47351093090;
    const double b1 = 23.08336743743;
    const double b2 = -21.06224101826;
    const double b3 =  3.13082909833;

    double t = std::sqrt(-2.0 * std::log(1.0 - p));   // t > 0

    // Approximation de la forme  t - (a0 + a1 t + a2 t² + a3 t³) /
    //                               (b0 + b1 t + b2 t² + b3 t³)
    double numerator   = ((a3 * t + a2) * t + a1) * t + a0;
    double denominator = ((b3 * t + b2) * t + b1) * t + b0;
    double z = t - numerator / denominator;

    if (neg) z = -z;          // on remet le signe si p était < 0.5

    // ---- 2. Retour à erfc⁻¹ ----
    // Φ⁻¹(p) = √2 * erfc⁻¹(2(1‑p))  ⇒  erfc⁻¹(x) = Φ⁻¹(1‑x/2) / √2
    return z / std::sqrt(2.0);
}

/* -----------------------------------------------------------------
   Inverse de la CDF normale standard Φ⁻¹(p) (p ∈ (0,1)).
   On ré‑utilise la même approximation que ci‑dessus, mais on
   évite le facteur √2 supplémentaire.
   ----------------------------------------------------------------- */
inline double normInv(double p)
{
    if (p <= 0.0) return -std::numeric_limits<double>::max();
    if (p >= 1.0) return  std::numeric_limits<double>::max();

    // Utilise la même logique que erfcInv, mais sans le facteur √2.
    bool neg = false;
    if (p < 0.5) {
        p = 1.0 - p;
        neg = true;
    }

    const double a0 =  2.50662823884;
    const double a1 = -18.61500062529;
    const double a2 =  41.39119773534;
    const double a3 = -25.44106049637;

    const double b0 = -8.47351093090;
    const double b1 = 23.08336743743;
    const double b2 = -21.06224101826;
    const double b3 =  3.13082909833;

    double t = std::sqrt(-2.0 * std::log(1.0 - p));

    double numerator   = ((a3 * t + a2) * t + a1) * t + a0;
    double denominator = ((b3 * t + b2) * t + b1) * t + b0;
    double z = t - numerator / denominator;

    return neg ? -z : z;
}

#pragma mark UCV

/* -----------------------------------------------------------------
   2.  Calcul du critère UCV pour une bande passante h donnée
   ----------------------------------------------------------------- */
double ucv_criterion(double h,
                     const std::vector<double>& x,
                     const std::size_t n)
{
    if (h <= 0.0) return std::numeric_limits<double>::infinity();

    double sum_pairs = 0.0;

#pragma omp parallel for reduction(+:sum_pairs) schedule(static)
    for (std::size_t i = 0; i < n - 1; ++i) {
        for (std::size_t j = i + 1; j < n; ++j) {
            const double delta = ((x[i] - x[j]) / h) * ((x[i] - x[j]) / h);
            sum_pairs += std::exp(-delta / 4.0) - std::sqrt(8.0) * std::exp(-delta / 2.0);
        }
    }

    // formule exacte du code C de R :
    // (0.5 + sum_pairs/n) / (n * h * sqrt(pi))
    return (0.5 + sum_pairs / static_cast<double>(n))
           / (static_cast<double>(n) * h * std::sqrt(M_PI));
}

/* -----------------------------------------------------------------
   3.  Recherche du minimum de UCV(h) – Brent (sans dépendance externe)
   ----------------------------------------------------------------- */
double golden_minimize(const std::vector<double>& x,
                      double a, double b,
                      double tol,
                      int max_iter)
{
    const std::size_t n = x.size();

    auto f = [&](double h) { return ucv_criterion(h, x, n); };

    const double phi = (std::sqrt(5.0) - 1.0) / 2.0;   // 0.618...
    double c  = b - phi * (b - a);
    double d  = a + phi * (b - a);
    double fc = f(c);
    double fd = f(d);

    for (int iter = 0; iter < max_iter; ++iter) {
        if (std::abs(b - a) < tol * (std::abs(c) + std::abs(d))) break;
        if (fc < fd) {
            b  = d;
            d  = c;
            fd = fc;
            c  = b - phi * (b - a);
            fc = f(c);
        } else {
            a  = c;
            c  = d;
            fc = fd;
            d  = a + phi * (b - a);
            fd = f(d);
        }
    }
    return 0.5 * (a + b);
}

double brent_minimize(const std::vector<double>& x,
                      double a, double b,
                      double tol,
                      int max_iter)
{
    const std::size_t n = x.size();
    auto f = [&](double h) { return ucv_criterion(h, x, n); };

    const double golden = (3.0 - std::sqrt(5.0)) / 2.0; // ≈ 0.3820

    double xm = 0.5 * (a + b);
    double v = xm, w = xm, x_cur = xm;
    double fv = f(v), fw = fv, fx = fv;
    double d = 0.0, e = 0.0;

    for (int iter = 0; iter < max_iter; ++iter) {
        xm = 0.5 * (a + b);
        const double tol1 = tol * std::abs(x_cur) + 1e-10;
        const double tol2 = 2.0 * tol1;

        // critère d'arrêt
        if (std::abs(x_cur - xm) <= tol2 - 0.5 * (b - a)) break;

        bool do_golden = true;

        // tentative d'interpolation parabolique
        if (std::abs(e) > tol1) {
            double r = (x_cur - w) * (fx - fv);
            double q = (x_cur - v) * (fx - fw);
            double p = (x_cur - v) * q - (x_cur - w) * r;
            q = 2.0 * (q - r);
            if (q > 0.0) p = -p; else q = -q;
            r = e;
            e = d;

            // accepter la parabole seulement si elle tombe dans [a,b]
            // et que le pas est suffisamment petit
            if (std::abs(p) < std::abs(0.5 * q * r) &&
                p > q * (a - x_cur) &&
                p < q * (b - x_cur))
            {
                d = p / q;
                double u = x_cur + d;
                if ((u - a) < tol2 || (b - u) < tol2)
                    d = (x_cur < xm) ? tol1 : -tol1;
                do_golden = false;
            }
        }

        // fallback golden section
        if (do_golden) {
            e = (x_cur < xm) ? b - x_cur : a - x_cur;
            d = golden * e;
        }

        const double u = x_cur + (std::abs(d) >= tol1 ? d : (d > 0 ? tol1 : -tol1));
        const double fu = f(u);

        // mise à jour de l'encadrement
        if (fu <= fx) {
            if (u < x_cur) b = x_cur; else a = x_cur;
            v = w; fv = fw;
            w = x_cur; fw = fx;
            x_cur = u; fx = fu;
        } else {
            if (u < x_cur) a = u; else b = u;
            if (fu <= fw || w == x_cur) { v = w; fv = fw; w = u; fw = fu; }
            else if (fu <= fv || v == x_cur || v == w) { v = u; fv = fu; }
        }
    }
    return x_cur;
}
/* -----------------------------------------------------------------
   4.  Interface publique : bw_ucv()
   ----------------------------------------------------------------- */
double bw_ucv(const std::vector<double>& data)
{
    if (data.empty())
        throw std::invalid_argument("bw_ucv : le vecteur d'entrée est vide");
    const std::size_t n = data.size();
    // -----------------------------------------------------------------
    // 4.1  Statistiques de base (moyenne, écart‑type) – utiles pour
    //      choisir un intervalle de recherche raisonnable.
    // -----------------------------------------------------------------
    const double mean = std::accumulate(data.begin(), data.end(), 0.0) / static_cast<double>(n);
    double var = 0.0;
    for (double v : data) {
        const double d = v - mean;
        var += d * d;
    }
    var /= static_cast<double>(n - 1);
    const double sigma = std::sqrt(var);
    // -----------------------------------------------------------------
    // 4.2  Point de départ : règle de Silverman (h_Silverman)
    // -----------------------------------------------------------------
    const double h_silver = 1.06 * sigma * std::pow(static_cast<double>(n), -0.2);
    // -----------------------------------------------------------------
    // 4.3  Définir un intervalle de recherche autour de h_silver.
    //      R utilise typiquement [h/3 , 3*h] (voir le code source de R).
    // -----------------------------------------------------------------
    const double lower = std::max(1e-6, h_silver / 3.0);
    const double upper = std::max(lower * 1.1, h_silver * 3.0); // on s’assure que upper>lower
    // -----------------------------------------------------------------
    // 4.4  Minimisation
    // -----------------------------------------------------------------
    const double h_opt = brent_minimize(data, lower, upper);
    return h_opt;
}

/* -----------------------------------------------------------------
   7.  Version naïve (pour vérification) – O(n²)
   ----------------------------------------------------------------- */
double bw_ucv_naive0(const std::vector<double>& data)
{
    const std::size_t n = data.size();
    if (n == 0) throw std::invalid_argument("empty data");
    // moyenne, sigma
    const double mean = std::accumulate(data.begin(), data.end(), 0.0) / static_cast<double>(n);
    double var = 0.0;
    for (double v : data) var += (v - mean) * (v - mean);
    var /= static_cast<double>(n - 1);
    const double sigma = std::sqrt(var);
    // règle de Silverman (point de départ)
    const double h0 = 1.06 * sigma * std::pow(static_cast<double>(n), -0.2);
    const double lower = std::max(1e-6, h0 / 3.0);
    const double upper = std::max(lower * 1.1, h0 * 3.0);
    // fonction UCV naïve
    auto ucv = [&](double h) -> double {
        const double inv_h = 1.0 / h;
        double sum = 0.0;
        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t j = i + 1; j < n; ++j) {
                const double u = (data[i] - data[j]) * inv_h;
                const double k = gaussian_kernel(u);
                sum += 2.0 * k;               // (i,j) et (j,i)
            }
        }
        const double term1 = sum / (static_cast<double>(n) * static_cast<double>(n) * h);
        const double term2 = 2.0 / (static_cast<double>(n) * h * std::sqrt(M_PI));
        return term1 - term2;
    };
    // Brent (identique à la version FFT)
    const double phi = (std::sqrt(5.0) - 1.0) / 2.0;
    double a = lower, b = upper;
    double c = b - phi * (b - a);
    double d = a + phi * (b - a);
    double fc = ucv(c);
    double fd = ucv(d);
    for (int iter = 0; iter < 100; ++iter) {
        if (std::abs(b - a) < 1e-8 * (std::abs(c) + std::abs(d))) break;
        if (fc < fd) {
            b = d; d = c; fd = fc;
            c = b - phi * (b - a);
            fc = ucv(c);
        } else {
            a = c; c = d; fc = fd;
            d = a + phi * (b - a);
            fd = ucv(d);
        }
    }
    return 0.5 * (a + b);
}

double bw_ucv_naive(const std::vector<double>& data)
{
    const std::size_t n = data.size();
    if (n < 2)
        throw std::invalid_argument("Need at least 2 points");

    // --------- stats de base (comme R) ---------
    const double mean =
        std::accumulate(data.begin(), data.end(), 0.0) / n;

    double var = 0.0;
    for (double v : data)
        var += (v - mean) * (v - mean);

    var /= (n - 1); // ✔️ comme R
    const double sigma = std::sqrt(var);

    // --------- point de départ (Silverman) ---------
    const double h0 = 1.06 * sigma * std::pow((double)n, -0.2);

    const double lower = std::max(1e-6, h0 / 3.0);
    const double upper = std::max(lower * 1.1, h0 * 3.0);

    // --------- noyau gaussien EXACT ---------
    auto K = [](double u) {
        static const double inv_sqrt_2pi =
            1.0 / std::sqrt(2.0 * M_PI);
        return inv_sqrt_2pi * std::exp(-0.5 * u * u);
    };

    // --------- fonction UCV ---------
    auto ucv = [&](double h) -> double {
        if (h <= 0.0)
            return std::numeric_limits<double>::infinity();

        const double inv_h = 1.0 / h;
        double sum = 0.0;

        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t j = i + 1; j < n; ++j) {

                const double u = (data[i] - data[j]) * inv_h;

                // ✔️ K_h
                const double kh = K(u) * inv_h;

                sum += 2.0 * kh;
            }
        }

        // ✔️ normalisation R
        const double term1 = sum / (n * (n - 1));

        // ✔️ terme exact convolution gaussienne
        const double term2 =
            1.0 / (2.0 * std::sqrt(M_PI) * n * h);

        return term1 - term2;
    };

    // --------- optimisation (golden section comme R) ---------
    const double phi = (std::sqrt(5.0) - 1.0) / 2.0;

    double a = lower;
    double b = upper;

    double c = b - phi * (b - a);
    double d = a + phi * (b - a);

    double fc = ucv(c);
    double fd = ucv(d);

    for (int iter = 0; iter < 100; ++iter) {
        if (std::abs(b - a) < 1e-8 * (std::abs(c) + std::abs(d)))
            break;

        if (fc < fd) {
            b = d;
            d = c;
            fd = fc;
            c = b - phi * (b - a);
            fc = ucv(c);
        } else {
            a = c;
            c = d;
            fc = fd;
            d = a + phi * (b - a);
            fd = ucv(d);
        }
    }

    return 0.5 * (a + b);
}
/* -----------------------------------------------------------------
   5.  Interface publique : bw_ucv_fft()
   ----------------------------------------------------------------- */

/* -----------------------------------------------------------------
   4.  Recherche du minimum (Brent) – même principe que la version
       O(n²) mais le critère est maintenant calculé via FFT.
erreur, C’est une recherche dorée.(golden‑section)
   ----------------------------------------------------------------- */
double brent_minimize(const UCV_FFT& ucv,
                      double a, double b,
                      double tol ,
                      int max_iter)
{
    const double phi = (std::sqrt(5.0) - 1.0) / 2.0;   // 0.618...
    double c = b - phi * (b - a);
    double d = a + phi * (b - a);
    double fc = ucv(c);
    double fd = ucv(d);
    for (int iter = 0; iter < max_iter; ++iter) {
        if (std::abs(b - a) < tol * (std::abs(c) + std::abs(d))) break;

        if (fc < fd) {
            b = d;
            d = c;
            fd = fc;
            c = b - phi * (b - a);
            fc = ucv(c);
        } else {
            a = c;
            c = d;
            fc = fd;
            d = a + phi * (b - a);
            fd = ucv(d);
        }
    }
    return 0.5 * (a + b);
}
/* -----------------------------------------------------------------
   5.  Interface publique : bw_ucv_fft()
   ----------------------------------------------------------------- */
double bw_ucv_fft(const std::vector<double>& data)
{
    if (data.empty())
        throw std::invalid_argument("bw_ucv_fft : empty input");
    const std::size_t n = data.size();
    // ---- 6.1  Statistiques de base (moyenne, écart‑type) ----
    const double mean = std::accumulate(data.begin(), data.end(), 0.0) / static_cast<double>(n);
    double var = 0.0;
    for (double v : data) {
        const double d = v - mean;
        var += d * d;
    }
    var /= static_cast<double>(n - 1);
    const double sigma = std::sqrt(var);

    // ---- 6.2  Point de départ : règle de Silverman (identique à R) ----
    const double h_silver = 1.06 * sigma * std::pow(static_cast<double>(n), -0.2);

    // ---- 6.3  Intervalle de recherche (h/3 , 3h) ----
    const double lower = std::max(1e-6, h_silver / 10.0);
    const double upper = std::max(lower * 1.1, h_silver * 3.0);
    // ---- 6.4  Objet UCV (FFT) ----
    const UCV_FFT ucv(data);
    // ---- 6.5  Minimisation ----
    const double h_opt = brent_minimize(ucv, lower, upper);
    return h_opt;
}

#pragma mark perpl


/*double ucv_score_gaussian(std::vector<double> x, double h)
{

    const int n = (int)x.size();
    if (n < 2 || h <= 0.0)
        return std::numeric_limits<double>::infinity();

    const double inv_h = 1.0 / h;

    double s1 = 0.0;
    double s2 = 0.0;

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {

            const double d = (x[i] - x[j]) * inv_h;

            // convolution gaussienne
            s1 += dnorm(d, 0, std::sqrt(2.0));

            if (i != j) {
                s2 += dnorm(d);
            }
        }
    }

    const double term1 =
        s1 / (n * n * h * std::sqrt(2.0));

    // 🔥 correction ici
    const double term2 =
       2* s2 / (n * (n - 1) * h);

    return term1 - term2;
}*/
double ucv_score_gaussian(std::vector<double> x, double h)
{
    const int n = (int)x.size();
    if (n < 2 || h <= 0.0)
        return std::numeric_limits<double>::infinity();

    double sum_pairs = 0.0;
    for (int i = 0; i < n - 1; ++i) {
        for (int j = i + 1; j < n; ++j) {
            const double delta = ((x[i] - x[j]) / h) * ((x[i] - x[j]) / h);
            sum_pairs += std::exp(-delta / 4.0) - std::sqrt(8.0) * std::exp(-delta / 2.0);
        }
    }

    // (0.5 + sum_pairs/n) / (n * h * sqrt(pi))
    return (0.5 + sum_pairs / n) / (n * h * std::sqrt(M_PI));
}

double bw_ucv_gaussian( std::vector<double> x,
                       double hmin,
                       double hmax,
                       int grid)
{
    double best_h = hmin;
    double best_score = std::numeric_limits<double>::infinity();

    const double log_min = std::log(hmin);
    const double log_max = std::log(hmax);

    for (int k = 0; k < grid; ++k) {
        double t = (grid == 1) ? 0.0 : (double)k / (grid - 1);
        double h = std::exp(log_min + t * (log_max - log_min));

        double score = ucv_score_gaussian(x, h);

        if (score < best_score) {
            best_score = score;
            best_h = h;
        }
    }
    return best_h;
}

double bw_SJ_dpi(const std::vector<double>& x) {
    const int    n     = x.size();
    const double sigma = scale_factor(x);   // min(sd, IQR/1.349)

    // --- Étape 1 : bandwidths pilotes ---
    // g1 : pilote pour estimer theta_44
    // formule R : 1.24 * sigma * n^(-1/7)
    const double g1 = 1.24 * sigma * std::pow((double)n, -1.0/7.0);

    // g2 : pilote pour estimer theta_24
    // formule R : 1.23 * sigma * n^(-1/9)
    const double g2 = 1.23 * sigma * std::pow((double)n, -1.0/9.0);

    // --- Étape 2 : estimation des fonctionnelles ---
    const double t44 = S4(x, g1);   // ≈ theta_44
    const double t24 = S6(x, g2);   // on utilise S6 pour theta_24...


    if (std::abs(t44) < 1e-15 || t44 >= 0.0)
        return 0.9 * sigma * std::pow((double)n, -0.2);

    if (std::abs(t24) < 1e-15)
        return 0.9 * sigma * std::pow((double)n, -0.2);

    const double TD = -t44;   // toujours > 0 ici
    const double SD =  t24;   // toujours défini ici


    const double alpha2 = 1.357 * std::pow(std::abs(SD / TD), 1.0/7.0);

    // theta_22(alpha2) estimé par S4
    const double t22 = S4(x, alpha2);

    if (t22 <= 0.0) {
        return 0.9 * sigma * std::pow((double)n, -0.2);
    }

    // h_DPI = (1 / (2*sqrt(pi)*n*t22))^(1/5)
    const double c1 = 1.0 / (2.0 * std::sqrt(M_PI) * n);
    return std::pow(c1 / t22, 0.2);
}


double bw_SJ_ste(const std::vector<double>& x, double tol, int max_iter )
{
    const int    n     = x.size();
    const double sigma = scale_factor(x);

    const double g1 = 1.24 * sigma * std::pow((double)n, -1.0/7.0);
    const double g2 = 1.23 * sigma * std::pow((double)n, -1.0/9.0);

    const int M = chooseFFtSize(n);
    const double t44 = S4_fft(x, g1, M);
    const double t24 = S6_fft(x, g2, M);

    if (std::abs(t44) < 1e-15 || std::abs(t24) < 1e-15)
        return 0.9 * sigma * std::pow((double)n, -0.2);

    const double alpha2 = 1.357 * std::pow(std::abs(t24 / t44), 1.0/7.0);
    const double c1     = 1.0 / (2.0 * std::sqrt(M_PI) * n);

    auto eq = [&](double h) { return sj_equation(h, x, alpha2, c1, M); };

    double lo = 0.1 * sigma * std::pow((double)n, -0.2);
    double hi = 2.0 * sigma * std::pow((double)n, -0.2);

    bool found = false;
    for (int k = 0; k < 20; k++) {
        if (eq(lo) * eq(hi) < 0.0) { found = true; break; }
        lo /= 1.2;
        hi *= 1.2;
    }
    if (!found)
        return 0.9 * sigma * std::pow((double)n, -0.2);

    double result = bisect(eq, lo, hi, tol, max_iter);

    if (std::isnan(result))
        return 0.9 * sigma * std::pow((double)n, -0.2);
    return result;
}


/*
double bw_SJ_ste(const std::vector<double>& x, double tol, int max_iter)
{
    const std::size_t n  = x.size();
    const double      nd = static_cast<double>(n);
    const double sigma   = scale_factor(x);
    const double h_ref   = 0.9 * sigma * std::pow(nd, -0.2);
    const int    M       = chooseFFtSize(n);

    // -----------------------------------------------------------------
    // Pilotes initiaux (S4 et S6 lancés en parallèle)
    // -----------------------------------------------------------------
    double t44 = 0.0, t24 = 0.0;

#pragma omp parallel sections
    {
#pragma omp section
        { t44 = S4_fft(x, 1.24 * sigma * std::pow(nd, -1.0 / 7.0), M); }

#pragma omp section
        { t24 = S6_fft(x, 1.23 * sigma * std::pow(nd, -1.0 / 9.0), M); }
    }

    if (std::abs(t44) < 1e-15 || std::abs(t24) < 1e-15)
        return h_ref;

    const double alpha2 = 1.357 * std::pow(std::abs(t24 / t44), 1.0 / 7.0);
    const double c1     = 1.0 / (2.0 * std::sqrt(M_PI) * nd);

    auto eq = [&](double h) { return sj_equation(h, x, alpha2, c1, M); };

    // -----------------------------------------------------------------
    // Étape 1 : balayage grossier (10 points) — parallélisé
    // -----------------------------------------------------------------
    const double log_lo = std::log(1e-4 * h_ref);
    const double log_hi = std::log(10.0 * h_ref);

    constexpr int N_COARSE = 10;
    std::array<double, N_COARSE + 1> cgrid, cevals;

    for (int k = 0; k <= N_COARSE; k++)
        cgrid[k] = std::exp(log_lo + k * (log_hi - log_lo) / N_COARSE);

#pragma omp parallel for schedule(dynamic)
    for (int k = 0; k <= N_COARSE; k++)
        cevals[k] = eq(cgrid[k]);

    // Collecter les intervalles suspects (changement de signe)
    std::vector<std::pair<double,double>> coarse_brackets;
    for (int k = 0; k < N_COARSE; k++) {
        if (std::isnan(cevals[k]) || std::isnan(cevals[k + 1])) continue;
        if (cevals[k] * cevals[k + 1] < 0.0)
            coarse_brackets.push_back({cgrid[k], cgrid[k + 1]});
    }

    if (coarse_brackets.empty())
        return h_ref;

    // -----------------------------------------------------------------
    // Étape 2 : raffinement local (10 points par intervalle suspect)
    // -----------------------------------------------------------------
    std::vector<std::pair<double,double>> fine_brackets;

    for (auto& [a, b] : coarse_brackets) {
        constexpr int N_FINE = 10;
        std::array<double, N_FINE + 1> fgrid, fevals;

        const double flog_lo = std::log(a);
        const double flog_hi = std::log(b);

        for (int k = 0; k <= N_FINE; k++)
            fgrid[k] = std::exp(flog_lo + k * (flog_hi - flog_lo) / N_FINE);

#pragma omp parallel for schedule(dynamic)
        for (int k = 0; k <= N_FINE; k++)
            fevals[k] = eq(fgrid[k]);

        for (int k = 0; k < N_FINE; k++) {
            if (std::isnan(fevals[k]) || std::isnan(fevals[k + 1])) continue;
            if (fevals[k] * fevals[k + 1] < 0.0)
                fine_brackets.push_back({fgrid[k], fgrid[k + 1]});
        }
    }

    if (fine_brackets.empty())
        return h_ref;

    // -----------------------------------------------------------------
    // Étape 3 : bissection sur chaque intervalle fin — parallélisée
    // -----------------------------------------------------------------
    const int nb = static_cast<int>(fine_brackets.size());
    std::vector<double> roots(nb, std::numeric_limits<double>::quiet_NaN());

#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < nb; i++) {
        const double root = bisect(eq, fine_brackets[i].first,
                                   fine_brackets[i].second, tol, max_iter);
        if (!std::isnan(root) && root > 0.0)
            roots[i] = root;
    }

    // Sélection de la racine la plus proche de h_ref en log-scale
    double best      = std::numeric_limits<double>::quiet_NaN();
    double best_dist = std::numeric_limits<double>::max();

    for (double root : roots) {
        if (std::isnan(root)) continue;
        const double dist = std::abs(std::log(root / h_ref));
        if (dist < best_dist) {
            best_dist = dist;
            best      = root;
        }
    }

    return std::isnan(best) ? h_ref : best;
}
*/