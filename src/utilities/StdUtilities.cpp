#include "StdUtilities.h"
#include <cmath>
#include <ctgmath>
#include <cstdlib>
#include <iostream>
#include <random>
#include <algorithm>
#include <QDebug>

using namespace std;

QList<float> reshape_trace(const QList<float>& trace)
{
    // Shift to have only positive values and then normalize.
    // They results will be in [0, 1] (symbolizing [minValue, maxValue])
    // (The x axis is in [0, numIter])
    
    float min = vector_min_value(trace);
    QList<float> reshaped = vector_shift_values_by(trace, min);
    return normalize_vector(reshaped);
}

QList<float> sample_vector(const QList<float>& aVector, const int numValues)
{
    QList<float> result;
    const int numOrg = (int)aVector.size();
    
    if(numOrg <= numValues)
    {
        result = aVector;
    }
    else
    {
        float values_per_point = numOrg / numValues;
        for(int i=0; i<numValues; ++i)
        {
            result.push_back(aVector[floor(i * values_per_point)]);
        }
    }
    return result;
}

QList<float> vector_shift_values_by(const QList<float>& aVector, const float v)
{
    QList<float> result;
    QList<float>::const_iterator it;
    for(it = aVector.begin(); it != aVector.end(); ++it)
    {
        result.push_back((*it) + v);
    }
    return result;
}

QList<float> create_histo(const QMap<float, float>& aMap, const int t_min, const int t_max)
{
    float max_value = map_max_value<float,float>(aMap); /** modif phd*/
    QList<float> histo;

    QMap<float, float>::const_iterator it;
    for(it = aMap.begin(); it != aMap.end(); ++it)
    {
        float t = it.key();
        float v = it.value();
        if(t <= t_max && t >= t_min)
        {
            histo.append(v / max_value);
        }
    }
    return histo;
}

QMap<float, float> vector_to_indexed_map(const QList<float>& aVector, const float minKey, const float maxKey)
{
    QMap<float, float> result;
    QList<float>::const_iterator it;
    
    float min = 0;
    float max = aVector.size();
    if(minKey != maxKey)
    {
        min = minKey;
        max = maxKey;
    }
    float diff = (max - min) / aVector.size();
    
    float i = 0.;
    for(it = aVector.begin(); it != aVector.end(); ++it)
    {
        //std::cout << min + i*diff << std::endl;
        result[min + i*diff] = *it;
        ++i;
    }
    return result;
}

QList<float> normalize_vector(const QList<float>& aVector)
{
    QList<float> histo;

    QList<float>::const_iterator it = max_element(aVector.begin(), aVector.end());
    if(it != aVector.end())
    {
        float max_value = *it;
        for(QList<float>::const_iterator it = aVector.begin(); it != aVector.end(); ++it)
        {
            histo.push_back((*it)/max_value);
        }
    }
    return histo;
}

QMap<float, float> normalize_map(const QMap<float, float>& aMap)
{
    float max_value = map_max_value(aMap);
    
    QMap<float, float> result;
    
    for(QMap<float, float>::const_iterator it = aMap.begin(); it != aMap.end(); ++it)
    {
        result[it.key()] = (it.value() / max_value);
    }
    return result;
}

QMap<float, float> equal_areas(const QMap<float, float>& mapToModify, const QMap<float, float>& mapWithTargetArea)
{
    QMapIterator<float, float> iter(mapWithTargetArea);
    float targetArea = 0.f;
    while(iter.hasNext())
    {
        iter.next();
        targetArea += iter.value();
    }
    return equal_areas(mapToModify, targetArea);
}

QMap<float, float> equal_areas(const QMap<float, float>& mapToModify, const float targetArea)
{
    QMapIterator<float, float> iter(mapToModify);
    float srcArea = 0.f;
    while(iter.hasNext())
    {
        iter.next();
        srcArea += iter.value();
    }
    float prop = targetArea / srcArea;
    
    QMap<float, float> result;
    QMapIterator<float, float> iter2(mapToModify);
    while(iter2.hasNext())
    {
        iter2.next();
        result[iter2.key()] = iter2.value() * prop;
    }
    return result;
}

float map_interpolate_key_for_value(const float value, const QMap<float, float>& aMap)
{
    for(QMap<float, float>::const_iterator it = aMap.begin(); it != aMap.end(); ++it)
    {
        if(it.value() > value)
        {
            float v2 = it.value();
            float k2 = it.key();
            --it;
            float v1 = it.value();
            float k1 = it.key();
            
            float k = k1 + (k2 - k1) * (value - v1) / (v2 - v1);
            
            //std::cout << "index : " << std::distance(aMap.begin(), it) << std::endl;
     
            return k;
        }
    }
    return 0;
    
    /*QMap<float, float>::const_iterator under = aMap.begin();
    QMap<float, float>::const_iterator above = aMap.begin();
    QMap<float, float>::const_iterator guess;
    
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
    
    float v2 = above->second;
    float k2 = above->first;
    
    float v1 = under->second;
    float k1 = under->first;
    
    float k = k1 + (k2 - k1) * (value - v1) / (v2 - v1);
    return k;*/
}

float map_interpolate_value_for_key(const float key, const QMap<float, float>& aMap)
{
    // method 1
    for(QMap<float, float>::const_iterator it = aMap.begin(); it != aMap.end(); ++it)
    {
        if(it.key() > key)
        {
            float v1 = it.value();
            float k1 = it.key();
            ++it;
            float v2 = it.value();
            float k2 = it.key();
            
            float v = v1 + (v2 - v1) * (key - k1) / (k2 - k1);
            return v;
        }
    }
    
    // method 2 : dichotomie ?
    /*std::pair<QMap<float, float>::const_iterator, QMap<float, float>::const_iterator> range = aMap.equal_range(key);
    if(range.first != range.second)
    {
        float v1 = range.first->second;
        float k1 = range.first->first;
        float v2 = range.second->second;
        float k2 = range.second->first;
        
        float v = v1 + (v2 - v1) * (key - k1) / (k2 - k1);
        return v;
    }*/
    
    return 0;
}


/** \brief Returns a map<int,float> with frequence sort by classes, as mathematical histogram sense
 * http://en.wikipedia.org/wiki/Histogram
* \param aClasse is the bins
*/
QMap<long int,long int> map_to_histogram(const QMap<long int, float>& aMap, long int aClasse=1)
{
    QMap<long int,long int> mClassFreq; /** first=Class second=frequence*/
    QMap<long int,float>::const_iterator it=aMap.begin();
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



QMap<long int, float> map_to_surface(const QMap<long int, float>& aMap, long int aClasse )
{
    QMap<long int,long int> mClassFreq; /** first=Class second=frequence*/

    QMap<long int,float>::const_iterator it=aMap.begin();
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
    QMap<long int, float> aMapSurface;
    it=aMap.begin();
    while(it!=aMap.end())
    {
        aMapSurface[it.key()]=(float)(mNbFreqCumul[it.value()])/(float)total;
        it++;
    }
    return aMapSurface;


}

QMap<float, float> map_to_surface(const QMap<float, float>& aMap, float aClasse)
{
    QMap<float,float> mClassFreq; /** first=Class second=frequence*/

    QMap<float,float>::const_iterator it = aMap.begin();
    float total=0;
    float nb_max=0;
    float v;
    
    /** calcul l'histogramme suivant la classe aClasse*/
    while(it!=aMap.end())
    {

        float param, fractpart, intpart;

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
    for(float i=0;i<=nb_max;i++)
    {
        mNbFreq[i]=num_iterations_for_value(mClassFreq,i);
    }*/
    QMap<float,float> mNbFreq;
    QMap<float,float>::iterator itNb=mClassFreq.begin();
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
    QMap<float,float> mNbFreqCumul;
    //QMap<float,float>::iterator itCumul=mNbFreqCumul.begin();
    //mNbFreqCumul[0]=0;
    //for(long int i=1;i<=nb_max;i++)
    itNb=mNbFreq.begin();
    float FreqCumul_1=0;
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
    QMap<float, float> aMapSurface;
    QMap<float, float>::iterator itClass=mClassFreq.begin();
    while(itClass!=mClassFreq.end())
    {
        aMapSurface[itClass.key()]=1-((float)(mNbFreqCumul[itClass.value()])/(float)total);
        itClass++;
    }
    return aMapSurface;
}



QList<pair<float,float>> HPD_from_surface(const QMap<float, float> &aMap, float aClasse, float threshold)
{
    QList<pair<float,float>> regions;
    long int a,b;

    QMap<float,float>::const_iterator its=aMap.begin();
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

const QMap<float, float> create_HPD(const QMap<float, float>& aMap, float /*aClasse*/, float threshold)
{
    QMap<float, float> inverted;
    QMapIterator<float, float> iter(aMap);
    float areaTot = 0.f;
    while(iter.hasNext())
    {
        iter.next();
        float t = iter.key();
        float v = iter.value();
        inverted[v] = t;
        areaTot += iter.value();
    }
    float areaSearched = areaTot * threshold / 100;
    
    QMap<float, float> result;
    QMapIterator<float, float> iter2(inverted);
    iter2.toBack();
    float area = 0.f;
    while(iter2.hasPrevious() && area < areaSearched)
    {
        iter2.previous();
        float t = iter2.value();
        float v = iter2.key();
        area += v;
        result[t] = v;
    }
    
    QMapIterator<float, float> iter3(aMap);
    while(iter3.hasNext())
    {
        iter3.next();
        float t = iter3.key();
        if(result.find(t) == result.end())
        {
            result[t] = 0.f;
        }
    }
    return result;
    
    
    /*QMap<float,float> input = normalize_map(aMap);
    
    // Aire totale
    float area_tot = 0;
    for(QMap<float,float>::const_iterator it = input.begin(); it != input.end(); ++it)
        area_tot += it.value();
    
    // Dichotomie
    float under = map_min_value(input);
    float above = map_max_value(input);
    
    float value = above;
    float area = area_tot;
    
    float limit_under = area_tot * (threshold-0.02);
    float limit_above = area_tot * (threshold+0.02);
    
    std::cout << "area_tot : " << area_tot << std::endl;
    std::cout << "threshold : " << threshold << std::endl;
    std::cout << "limit under : " << limit_under << std::endl;
    std::cout << "limit above : " << limit_above << std::endl;
    
    while(area < limit_under || area > limit_above)
    {
        value = under + (above - under) / 2;
        area = 0;
        for(QMap<float,float>::const_iterator it = input.begin(); it != input.end(); ++it)
        {
            if(it.value() > value)
                area += it.value();
        }
        
        std::cout << "---------" << std::endl;
        std::cout << "under : " << under << std::endl;
        std::cout << "above : " << above << std::endl;
        std::cout << "value : " << value << std::endl;
        std::cout << "area : " << area << std::endl;
        std::cout << "area percent : " << area / area_tot << std::endl;
        
        if(area / area_tot < threshold)
        {
            std::cout << "GO DOWN" << std::endl;
            above = value;
        }
        else
        {
            std::cout << "GO UP" << std::endl;
            under = value;
        }
    }
    std::cout << "OK OK OK OK OK OK OK OK" << std::endl;
    std::cout << "---------" << std::endl;
    std::cout << "Found value : " << value << std::endl;
    std::cout << "Area Percent : " << area / area_tot << std::endl;
    std::cout << "Area : " << area << std::endl;
    std::cout << "---------" << std::endl;
    
    QMap<float, float> bounds;
    QMap<float, float> result;
    bool is_under = true;
    float start = 0;
    float end = 0;
    for(QMap<float,float>::const_iterator it = input.begin(); it != input.end(); ++it)
    {
        if(is_under && it.value() > value)
        {
            is_under = false;
            
            //float v2 = it.value();
            //float k2 = it.key();
            //std::cout << "Cross above at : " << k2 << ", " << v2 << std::endl;
            //--it;
            //float v1 = it.value();
            //float k1 = it.key();
            //++it;
            //float k = k1 + (k2 - k1) * (value - v1) / (v2 - v1);
            //results[k] = value;
            
            //results[it.key()] = it.value();
            
            start = it.key();
            std::cout << "Start : " << start << std::endl;
        }
        else if(!is_under && it.value() <= value)
        {
            is_under = true;
            
            //float v1 = it.value();
            //float k1 = it.key();
            //++it;
            //float v2 = it.value();
            //float k2 = it.key();
            //--it;
            //float k = k1 + (k2 - k1) * (value - v1) / (v2 - v1);
            //results[k] = value;
            
            end = it.key();
            std::cout << "End : " << end << std::endl;
            
            bounds[start] = end;
            std::cout << "----------" << std::endl;
        }
        
        if(it.value() > value)
        {
            result[it.key()] = it.value();
        }
        else
        {
            result[it.key()] = 0;
        }
    }
    return result;*/
}




