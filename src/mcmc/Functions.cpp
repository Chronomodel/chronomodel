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
    
    float variance = (sum2 / sumP) - powf(result.mean, 2);
    
    if(variance < 0)
        variance = 0;
    
    result.stddev = sqrtf(variance);
    
    return result;
}

float dataStd(const QVector<float>& data)
{
    // Work with double precision here because sum2 might be big !
    
    double sum = 0.;
    double sum2 = 0.;
    
    for(int i=0; i<data.size(); ++i)
    {
        double x = (double)data[i];
        sum += x;
        sum2 += x * x;
    }
    
    double mean = sum / data.size();
    double variance = sum2 / data.size() - mean * mean;
    
    /*qDebug() << "sum : " << sum;
    qDebug() << "sum2 : " << sum2;
    qDebug() << "size : " << data.size();
    qDebug() << "mean : " << mean;
    qDebug() << "variance : " << variance;
    qDebug() << "std : " << sqrtf(variance);*/
    
    if(variance < 0)
    {
        for(int i=0; i<20; ++i)
        {
            qDebug() << "data " << i << " : "<< data[i];
        }
        
        qDebug() << "ERROR : negative variance found : " << variance;
        return 0.f;
    }
    return (float)sqrt(variance);
}

QString functionAnalysisToString(const FunctionAnalysis& analysis)
{
    QString result;
    int precision = 0;
    
    result += "Mode : " + QString::number(analysis.mode, 'f', precision) + "   ";
    result += "Mean : " + QString::number(analysis.mean, 'f', precision) + "   ";
    result += "Std deviation : " + QString::number(analysis.stddev, 'f', precision) + "\n";
    
    return result;
}

QString densityAnalysisToString(const DensityAnalysis& analysis)
{
    QString result = functionAnalysisToString(analysis.analysis);
    int precision = 0;
    
    result += "Q1 : " + QString::number(analysis.quartiles.Q1, 'f', precision) + "   ";
    result += "Q2 (Median) : " + QString::number(analysis.quartiles.Q2, 'f', precision) + "   ";
    result += "Q3 : " + QString::number(analysis.quartiles.Q3, 'f', precision) + "\n";
    
    return result;
}