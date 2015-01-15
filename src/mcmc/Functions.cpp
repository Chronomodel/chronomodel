#include "Functions.h"
#include "Generator.h"
#include "StdUtilities.h"
#include <QDebug>

// -----------------------------------------------------------------
//  sumP = Sum (pi)
//  sum = Sum (pi * xi)
//  sum2 = Sum (pi * xi^2)
// -----------------------------------------------------------------

// TODO : handle empty function case and null density case (pi = 0)

FunctionAnalysis analyseFunction(const QMap<double, double>& aFunction)
{
    typename QMap<double, double>::const_iterator it;
    
    double max = 0;
    double mode = 0;
    double sum = 0.;
    double sum2 = 0.;
    double sumP = 0.;
    
    double prevY = 0;
    QList<double> uniformXValues;
    
    QMapIterator<double, double> iter(aFunction);
    while(iter.hasNext())
    {
        iter.next();
        
        double x = iter.key();
        double y = iter.value();
        
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
    
    double variance = (sum2 / sumP) - pow(result.mean, 2);
    
    if(variance < 0)
        variance = 0;
    
    result.stddev = sqrt(variance);
    
    return result;
}

double dataStd(const QVector<double>& data)
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
    qDebug() << "std : " << sqrt(variance);*/
    
    if(variance < 0)
    {
        qDebug() << "ERROR : negative variance found : " << variance;
        return 0.f;
    }
    return (double)sqrt(variance);
}

double shrinkageUniform(double so2)
{
    double u = Generator::randomUniform();
    return (so2 * (1.f - u) / u);
}

QString functionAnalysisToString(const FunctionAnalysis& analysis)
{
    QString result;
    int precision = 0;
    
    result += "MAP : " + QString::number(analysis.mode, 'f', precision) + "   ";
    result += "Mean : " + QString::number(analysis.mean, 'f', precision) + "   ";
    result += "Std deviation : " + QString::number(analysis.stddev, 'f', precision) + "<br>";
    
    return result;
}

QString densityAnalysisToString(const DensityAnalysis& analysis)
{
    QString result = functionAnalysisToString(analysis.analysis);
    int precision = 2;
    
    result += "Q1 : " + QString::number(analysis.quartiles.Q1, 'f', precision) + "   ";
    result += "Q2 (Median) : " + QString::number(analysis.quartiles.Q2, 'f', precision) + "   ";
    result += "Q3 : " + QString::number(analysis.quartiles.Q3, 'f', precision) + "<br>";
    
    return result;
}

Quartiles quartilesForTrace(const QVector<double>& trace)
{
    Quartiles quartiles;
    
    QVector<double> sorted = trace;
    qSort(sorted);
    
    int q1index = ceil((double)sorted.size() * 0.25f);
    int q3index = ceil((double)sorted.size() * 0.75f);
    
    quartiles.Q1 = sorted[q1index];
    quartiles.Q3 = sorted[q3index];
    
    if(sorted.size() % 2 == 0)
    {
        int q2indexLow = sorted.size() / 2;
        int q2indexUp = q2indexLow + 1;
        
        quartiles.Q2 = sorted[q2indexLow] + (sorted[q2indexUp] - sorted[q2indexLow]) / 2.f;
    }
    else
    {
        int q2index = ceil((double)sorted.size() * 0.5f);
        quartiles.Q2 = sorted[q2index];
    }
    return quartiles;
}

Quartiles quartilesForRepartition(const QVector<double>& repartition, double tmin, double step)
{
    Quartiles quartiles;
    
    double q1index = vector_interpolate_idx_for_value(0.25, repartition);
    double q2index = vector_interpolate_idx_for_value(0.5, repartition);
    double q3index = vector_interpolate_idx_for_value(0.75, repartition);
    
    quartiles.Q1 = tmin + q1index * step;
    quartiles.Q2 = tmin + q2index * step;
    quartiles.Q3 = tmin + q3index * step;
    
    return quartiles;
}

QPair<double, double> credibilityForTrace(const QVector<double>& trace, int thresh, double& exactThresholdResult)
{
    QPair<double, double> credibility;
    credibility.first = 0;
    credibility.second = 0;
    exactThresholdResult = 0;
    
    if(thresh > 0 && trace.size() > 0)
    {
        int threshold = qMin(thresh, 100);
        
        QVector<double> sorted = trace;
        qSort(sorted);
        
        int numToRemove = floor((double)sorted.size() * (1.f - (double)threshold / 100.f));
        exactThresholdResult = ((double)sorted.size() - (double)numToRemove) / (double)sorted.size();
        
        int k = numToRemove;
        int n = sorted.size();
        double lmin = 0.f;
        int foundJ = 0;
        for(int j=0; j<=k; ++j)
        {
            double l = sorted[(n - 1) - k + j] - sorted[j];
            if(lmin == 0.f || l < lmin)
            {
                foundJ = j;
                lmin = l;
            }
        }
        credibility.first = sorted[foundJ];
        credibility.second = sorted[(n - 1) - k + foundJ];
    }
    
    return credibility;
}

QString intervalText(const QPair<double, double>& interval)
{
    return "[" + QString::number(interval.first, 'f', 0) + ", " + QString::number(interval.second, 'f', 0) + "]";
}

QString getHPDText(const QMap<double, double>& hpd)
{
    QList<QPair<double, double>> intervals = intervalsForHpd(hpd);
    
    QStringList results;
    for(int i=0; i<intervals.size(); ++i)
    {
        results << intervalText(intervals[i]);
    }
    QString result = results.join(" ");
    return result;
}

QList<QPair<double, double>> intervalsForHpd(const QMap<double, double>& hpd)
{
    QMapIterator<double, double> it(hpd);
    QList<QPair<double, double>> intervals;
    
    bool inInterval = false;
    double lastKeyInInter = 0.;
    QPair<double, double> curInterval;
    
    while(it.hasNext())
    {
        it.next();
        if(it.value() != 0 && !inInterval)
        {
            inInterval = true;
            curInterval.first = it.key();
            lastKeyInInter = it.key();
        }
        else if(inInterval)
        {
            if(it.value() == 0)
            {
                inInterval = false;
                curInterval.second = lastKeyInInter;
                intervals.append(curInterval);
            }
            else
            {
                lastKeyInInter = it.key();
            }
        }
    }
    return intervals;
}