#include "Functions.h"
#include <cmath>
#include <QDebug>

// -----------------------------------------------------------------
//  sumP = Sum (pi)
//  sum = Sum (pi * xi)
//  sum2 = Sum (pi * xi^2)
// -----------------------------------------------------------------

// TODO : handle empty function case and null density case (pi = 0)

FunctionAnalysis analyseFunction(const QMap<float, float>& aFunction)
{
    typename QMap<float, float>::const_iterator it;
    
    float max = 0;
    float mode = 0;
    float sum = 0.;
    float sum2 = 0.;
    float sumP = 0.;
    
    float prevY = 0;
    QList<float> uniformXValues;
    
    QMapIterator<float, float> iter(aFunction);
    while(iter.hasNext())
    {
        iter.next();
        
        float x = iter.key();
        float y = iter.value();
        
        sumP += y;
        sum += y * x;
        sum2 += y * x * x;
        
        if(max <= y)
        {
            max = y;
            if(prevY == y)
            {
                uniformXValues.append(x);
                int middleIndex = floor(uniformXValues.size()/2);
                mode = uniformXValues[middleIndex];
            }
            else
            {
                uniformXValues.clear();
                mode = x;
            }
        }
        prevY = y;
        
        
        
        // TODO
        /*if((*it).second == prev)
         {
         uniformFound = true;
         uniformValue = prev;
         }
         else
         {
         if(uniformFound)
         {
         //mode = moyenne sur l'uniforme
         }
         uniformFound = false;
         }
         
         prev = (*it).second;*/
    }
    
    FunctionAnalysis result;
    result.max = max;
    result.mode = mode;
    result.mean = sum / sumP;
    result.variance = (sum2 / sumP) - pow(result.mean, 2);
    
    if(result.variance < 0)
        result.variance = 0;
    
    return result;
}

float dataStd(const QVector<float>& data)
{
    float sum = 0.;
    float sum2 = 0.;
    
    for(int i=0; i<data.size(); ++i)
    {
        float x = data[i];
        sum += x;
        sum2 += x * x;
    }
    
    float mean = sum / data.size();
    float variance = (sum2 / data.size()) - pow(mean, 2);
    
    return sqrt(variance);
}
