#include "StdUtilities.h"
#include <cmath>
#include <ctgmath>
#include <cstdlib>
#include <iostream>
#include <random>
#include <algorithm>
#include <QDebug>

using namespace std;

QList<double> reshape_trace(const QList<double>& trace)
{
    // Shift to have only positive values and then normalize.
    // They results will be in [0, 1] (symbolizing [minValue, maxValue])
    // (The x axis is in [0, numIter])
    
    double min = vector_min_value(trace);
    QList<double> reshaped = vector_shift_values_by(trace, min);
    return normalize_list(reshaped);
}

QList<double> sample_vector(const QList<double>& aVector, const int numValues)
{
    QList<double> result;
    const int numOrg = (int)aVector.size();
    
    if(numOrg <= numValues)
    {
        result = aVector;
    }
    else
    {
        double values_per_point = numOrg / numValues;
        for(int i=0; i<numValues; ++i)
        {
            result.push_back(aVector[floor(i * values_per_point)]);
        }
    }
    return result;
}

QList<double> vector_shift_values_by(const QList<double>& aVector, const double v)
{
    QList<double> result;
    QList<double>::const_iterator it;
    for(it = aVector.begin(); it != aVector.end(); ++it)
    {
        result.push_back((*it) + v);
    }
    return result;
}

QList<double> create_histo(const QMap<double, double>& aMap, const int t_min, const int t_max)
{
    double max_value = map_max_value<double,double>(aMap); /** modif phd*/
    QList<double> histo;

    QMap<double, double>::const_iterator it;
    for(it = aMap.begin(); it != aMap.end(); ++it)
    {
        double t = it.key();
        double v = it.value();
        if(t <= t_max && t >= t_min)
        {
            histo.append(v / max_value);
        }
    }
    return histo;
}

QMap<double, double> vector_to_indexed_map(const QList<double>& aVector, const double minKey, const double maxKey)
{
    QMap<double, double> result;
    QList<double>::const_iterator it;
    
    double min = 0;
    double max = aVector.size();
    if(minKey != maxKey)
    {
        min = minKey;
        max = maxKey;
    }
    double diff = (max - min) / aVector.size();
    
    double i = 0.;
    for(it = aVector.begin(); it != aVector.end(); ++it)
    {
        //std::cout << min + i*diff << std::endl;
        result[min + i*diff] = *it;
        ++i;
    }
    return result;
}

QList<double> normalize_list(const QList<double>& aVector)
{
    QList<double> histo;

    QList<double>::const_iterator it = max_element(aVector.begin(), aVector.end());
    if(it != aVector.end())
    {
        double max_value = *it;
        for(QList<double>::const_iterator it = aVector.begin(); it != aVector.end(); ++it)
        {
            histo.push_back((*it)/max_value);
        }
    }
    return histo;
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
    
    double srcArea = 0.f;
    
    //qDebug() << "------";
    
    while(iter.hasNext())
    {
        iter.next();
        double t = iter.key();
        double v = iter.value() * (t - lastT);
        //qDebug() << t << ", " << v;
        srcArea += v;
        lastT = t;
    }
    double prop = targetArea / srcArea;
    /*qDebug() << "Equal_areas prop = " << prop;
    qDebug() << "targetArea = " << targetArea;
    qDebug() << "srcArea = " << srcArea;*/
    
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
    for(int i=0; i<data.size(); ++i)
        srcArea += step * data[i];
    
    double prop = area / srcArea;
    QVector<double> result;
    for(int i=0; i<data.size(); ++i)
        result.append(data[i] * prop);
    
    return result;
}

QMap<double, double> vector_to_map(const QVector<double>& data, const double min, const double max, const double step)
{
    QMap<double, double> map;
    int nbPts = 1 + (int)round((max - min) / step);
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

double map_interpolate_key_for_value(const double value, const QMap<double, double>& map)
{
    // Dichotomie
    
    if(map.key(value, -99999) != -99999)
    {
        return map.key(value);
    }
    else
    {
        double keyInf = map.firstKey();
        double keySup = map.lastKey();
        do
        {
            double keyMid = keyInf + floor((keySup - keyInf) / 2.f);
            double valueMid;
            QMap<double, double>::const_iterator it = map.find(keyMid);
            if(it != map.end())
                valueMid = it.value();
            else
            {
                double valueMidInf = map.value(floor(keyMid));
                double valueMidSup = map.value(ceil(keyMid));
                valueMid = valueMidInf + (valueMidSup - valueMidInf) / 2.f;
            }
            if(value < valueMid)
                keySup = keyMid;
            else
                keyInf = keyMid;
            
            //qDebug() << keyInf << ", " << keySup;
            
        }while(keySup - keyInf > 1);
        
        double valueInf = map.value(keyInf);
        double valueSup = map.value(keySup);
        double prop = (value - valueInf) / (valueSup - valueInf);
        double key = keyInf + prop * (keySup - keyInf);
        
        //qDebug() << key;
        
        return key;
    }
    
    
    // Old school
    
    /*for(QMap<double, double>::const_iterator it = map.begin(); it != map.end(); ++it)
    {
        if(it.value() > value)
        {
            double v2 = it.value();
            double k2 = it.key();
            --it;
            double v1 = it.value();
            double k1 = it.key();
            
            double k = k1 + (k2 - k1) * (value - v1) / (v2 - v1);
            
            //std::cout << "index : " << std::distance(aMap.begin(), it) << std::endl;
     
            return k;
        }
    }
    return 0;*/
    
    // ----
    
    /*QMap<double, double>::const_iterator under = aMap.begin();
    QMap<double, double>::const_iterator above = aMap.begin();
    QMap<double, double>::const_iterator guess;
    
    std::advance(above, aMap.size());
    
    int underIndex = std::distance(aMap.begin(), under);
    int aboveIndex = std::distance(aMap.begin(), above);
    
    int counter = 0;
    
    while(aboveIndex - underIndex > 1)
    {
        underIndex = std::distance(aMap.begin(), under);
        aboveIndex = std::distance(aMap.begin(), above);
        
        //std::cout << "under : " << underIndex << std::endl;
        //std::cout << "above : " << aboveIndex << std::endl;
        //std::cout << "guess index : " << (aboveIndex - underIndex) / 2 << std::endl;
        //std::cout << "guess value : " << guess->second << std::endl;
        //std::cout << "value : " << value << std::endl;
        //std::cout << "---------" << std::endl;
        
        guess = aMap.begin();
        std::advance(guess, underIndex + (aboveIndex - underIndex) / 2);
        
        if(guess->second > value)
        {
            above = guess;
        }
        else
        {
            under = guess;
        }
        ++counter;
    }
    
    std::cout << "dichotomy trials : " << counter << std::endl;
    
    double v2 = above->second;
    double k2 = above->first;
    
    double v1 = under->second;
    double k1 = under->first;
    
    double k = k1 + (k2 - k1) * (value - v1) / (v2 - v1);
    return k;*/
}

double map_interpolate_value_for_key(const double key, const QMap<double, double>& map)
{
    QMap<double, double>::const_iterator iter = map.find(key);
    if(iter != map.end())
    {
        return map.value(key);
    }
    else
    {
        // Weird but exact :
        QMap<double, double>::const_iterator iterAfter = map.lowerBound(key);
        QMap<double, double>::const_iterator iterBefore = iterAfter - 1;
        if(iterBefore != map.begin())
        {
            double valueBefore = iterBefore.value();
            double valueAfter = iterAfter.value();
            return valueBefore + (valueAfter - valueBefore) / 2;
        }
        else
        {
            return iterAfter.value();
        }
    }
    
    // method 1
    /*for(QMap<double, double>::const_iterator it = aMap.begin(); it != aMap.end(); ++it)
    {
        if(it.key() > key)
        {
            double v1 = it.value();
            double k1 = it.key();
            ++it;
            double v2 = it.value();
            double k2 = it.key();
            
            double v = v1 + (v2 - v1) * (key - k1) / (k2 - k1);
            return v;
        }
    }*/
    
    // method 2 : dichotomie ?
    /*std::pair<QMap<double, double>::const_iterator, QMap<double, double>::const_iterator> range = aMap.equal_range(key);
    if(range.first != range.second)
    {
        double v1 = range.first->second;
        double k1 = range.first->first;
        double v2 = range.second->second;
        double k2 = range.second->first;
        
        double v = v1 + (v2 - v1) * (key - k1) / (k2 - k1);
        return v;
    }*/
    
    return 0;
}


/** \brief Returns a map<int,double> with frequence sort by classes, as mathematical histogram sense
 * http://en.wikipedia.org/wiki/Histogram
* \param aClasse is the bins
*/
QMap<long int,long int> map_to_histogram(const QMap<long int, double>& aMap, long int aClasse=1)
{
    QMap<long int,long int> mClassFreq; /** first=Class second=frequence*/
    QMap<long int,double>::const_iterator it=aMap.begin();
    while(it!=aMap.end())
    {
        ldiv_t divresult = std::div((it.key()),aClasse);

        if(mClassFreq.find(divresult.quot) == mClassFreq.end())
        {
           mClassFreq[divresult.quot] = it.value();
        }
        else
        {
            mClassFreq[divresult.quot] = mClassFreq[divresult.quot] + it.value();
        }
        it++;
    }

    return mClassFreq;
}



QMap<long int, double> map_to_surface(const QMap<long int, double>& aMap, long int aClasse )
{
    QMap<long int,long int> mClassFreq; /** first=Class second=frequence*/

    QMap<long int,double>::const_iterator it=aMap.begin();
    long int total=0;
    long int nb_max=0;
    while(it!=aMap.end())
    {
        ldiv_t divresult = std::div((it.key()),aClasse);

        if(mClassFreq.find(divresult.quot) == mClassFreq.end())
        {
           mClassFreq[divresult.quot] = it.value();
        }
        else
        {
            mClassFreq[divresult.quot] = mClassFreq[divresult.quot] + it.value();
        }
        nb_max=(mClassFreq[divresult.quot]>nb_max ? mClassFreq[divresult.quot]:nb_max);
        total += it.value();
        it++;
    }
    /** détermine la frequence des frequences*/
    QMap<long int,long int> mNbFreq;
    for(long int i=0;i<=nb_max;i++)
    {
        mNbFreq[i]=num_iterations_for_value(mClassFreq,i);
    }
    /** calcul l'histogramme cumulé des fréquences des fréquences, cela permet d'obtenir la surface correspondant
     * ? chaque fréquence
     * */
    QMap<long int,long int> mNbFreqCumul;
    mNbFreqCumul[0]=0;
    for(long int i=1;i<=nb_max;i++)
    {
        mNbFreqCumul[i]=mNbFreq[i]*i+mNbFreqCumul[i-1];
    }
    /** cree une copie de la map d'entree en remplacant les frequence par leur isoSurface ex: 45= 0.95
     *
     */
    std::cout<<"total"<<total<<std::endl;
    QMap<long int, double> aMapSurface;
    it=aMap.begin();
    while(it!=aMap.end())
    {
        aMapSurface[it.key()]=(double)(mNbFreqCumul[it.value()])/(double)total;
        it++;
    }
    return aMapSurface;


}

QMap<double, double> map_to_surface(const QMap<double, double>& aMap, double aClasse)
{
    QMap<double,double> mClassFreq; /** first=Class second=frequence*/

    QMap<double,double>::const_iterator it = aMap.begin();
    double total=0;
    double nb_max=0;
    double v;
    
    /** calcul l'histogramme suivant la classe aClasse*/
    while(it!=aMap.end())
    {

        double param, fractpart, intpart;

        param = (it.key())/ aClasse;
        fractpart = modf (param , &intpart);
        if(mClassFreq.find(intpart) == mClassFreq.end())
        {
            v=it.value();
           mClassFreq[intpart] = it.value();
        }
        else
        {
             v=it.value();
            mClassFreq[intpart] = mClassFreq[intpart] + it.value();
        }
        nb_max=(mClassFreq[intpart]>nb_max ? mClassFreq[intpart]:nb_max);


        total += it.value();
        it++;
    }
    /** détermine la frequence des frequences*/
   /* QMap<long int,long int> mNbFreq;
    for(double i=0;i<=nb_max;i++)
    {
        mNbFreq[i]=num_iterations_for_value(mClassFreq,i);
    }*/
    QMap<double,double> mNbFreq;
    QMap<double,double>::iterator itNb=mClassFreq.begin();
    while(itNb!=mClassFreq.end())
    {
        v=itNb.value();
        if(mNbFreq.find(v) == mNbFreq.end())
        {
            mNbFreq[v]=num_iterations_for_value(mClassFreq,v);
           // std::cout<<"v="<<v<<" mNbFreq[v]"<<mNbFreq[v]<<std::endl;
        }
        itNb++;
    }

    /** calcul l'histogramme cumulé des fréquences des fréquences, cela permet d'obtenir la surface correspondant
     * à chaque fréquence
     * */
    QMap<double,double> mNbFreqCumul;
    //QMap<double,double>::iterator itCumul=mNbFreqCumul.begin();
    //mNbFreqCumul[0]=0;
    //for(long int i=1;i<=nb_max;i++)
    itNb=mNbFreq.begin();
    double FreqCumul_1=0;
    while(itNb!=mNbFreq.end())
    {
        v=itNb.key();
        mNbFreqCumul[v]=mNbFreq[v]*v+FreqCumul_1;//La map range automatiquement les key dans l'ordre croissant
        FreqCumul_1=mNbFreqCumul[v];
        itNb++;
       //  std::cout<<"v="<<v<<" mNbFreqCumul[v]"<<mNbFreqCumul[v]<<std::endl;
    }
    /** cree une copie de la map d'entree en remplacant les frequences par leur isoSurface ex: 45= 0.95
     *
     */
    QMap<double, double> aMapSurface;
    QMap<double, double>::iterator itClass=mClassFreq.begin();
    while(itClass!=mClassFreq.end())
    {
        aMapSurface[itClass.key()]=1-((double)(mNbFreqCumul[itClass.value()])/(double)total);
        itClass++;
    }
    return aMapSurface;
}



QList<pair<double,double>> HPD_from_surface(const QMap<double, double> &aMap, double aClasse, double threshold)
{
    QList<pair<double,double>> regions;
    long int a,b;

    QMap<double,double>::const_iterator its=aMap.begin();
    while(its!=aMap.end())
      {
        a=its.key();
        
        if(its.value()<=threshold)
        {

            do
            {
                its++;
                b=its.key();


            }while( (its.value()<=threshold) && (its!=aMap.end()) );
            regions.push_back(std::make_pair(a*aClasse,b*aClasse));
        }
        if(its!=aMap.end()) its++;

    } ;

    return regions;
}

const QMap<double, double> create_HPD(const QMap<double, double>& aMap, double /*aClasse*/, double threshold)
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
    double area = 0.f;
    while(iter.hasPrevious())
    {
        iter.previous();
        double t = iter.value();
        double v = iter.key();
        area += v;
        result[t] = (area < areaSearched) ? v : 0;
    }
    return result;
}




