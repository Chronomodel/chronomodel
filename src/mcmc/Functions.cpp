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
    }
    
    FunctionAnalysis result;
    result.max = max;
    result.mode = mode;
    result.mean = 0;
    result.stddev = 0;
    
    if(sumP != 0)
    {
        result.mean = sum / sumP;
        double variance = (sum2 / sumP) - pow(result.mean, 2);
        
        if(variance < 0) {
            qDebug() << "WARNING : in analyseFunction() negative variance found : " << variance<<" return 0";
            variance = 0;
        }
        
        result.stddev = sqrt(variance);
    }
    
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
        qDebug() << "WARNING : in dataStd() negative variance found : " << variance<<" return 0";
        return 0.f;
    }
    return (double)sqrt(variance);
}

double shrinkageUniform(double so2)
{
    double u = Generator::randomUniform();
    return (so2 * (1. - u) / u);
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

QPair<double, double> credibilityForTrace(const QVector<double>& trace, double thresh, double& exactThresholdResult)
{
    QPair<double, double> credibility;
    credibility.first = 0;
    credibility.second = 0;
    exactThresholdResult = 0;
    
    if(thresh > 0 && trace.size() > 0)
    {
        //int threshold = qMin(thresh, 100);
        double threshold =  (thresh > 100 ? thresh = 100.0 : thresh);
        threshold = (thresh < 0 ? thresh = 0.0 : thresh);
        QVector<double> sorted = trace;
        qSort(sorted);
        
        //int numToRemove = floor((double)sorted.size() * (1.f - (double)threshold / 100.f));
        int numToRemove = floor((double)sorted.size() * (1.f - threshold / 100.f));
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

QString intervalText(const QPair<double, QPair<double, double> >& interval, bool isDate)
{    
    if(isDate){
        return "[" + doubleInStrDate(interval.second.first) + "; " + doubleInStrDate(interval.second.second) + "] (" + QString::number(interval.first, 'f', 1 ) + "%)";
    }
    else {
        return "[" + QString::number(interval.second.first, 'f', 1) + "; " + QString::number(interval.second.second, 'f', 1) + "] (" + QString::number(interval.first, 'f', 1 ) + "%)";
    }
    
}

QString getHPDText(const QMap<double, double>& hpd, double thresh, bool isDate)
{
    QList<QPair<double, QPair<double, double> > > intervals = intervalsForHpd(hpd, thresh);
    
    QStringList results;
    for(int i=0; i<intervals.size(); ++i)
    {
        results << intervalText(intervals[i],isDate);
    }
    QString result = results.join(", ");
    if (isDate) {
        result += " "+dateFormat();
    }
    return result;
}

QList<QPair<double, QPair<double, double> > > intervalsForHpd(const QMap<double, double>& hpd, double thresh)
{
    QMapIterator<double, double> it(hpd);
    QList<QPair<double, QPair<double, double> > > intervals;
    
    bool inInterval = false;
    double lastKeyInInter = 0.;
    QPair<double, double> curInterval;
    
    /* original code HL
     double areaTot = 0;
    while(it.hasNext())
    {
        it.next();
        areaTot += it.value();
    }
     */
    
    double areaTot= map_area(hpd); // modif PhD on 2015/05/20
    double lastValueInInter = 0.;
    
    double areaCur = 0;
    it.toFront();
    while(it.hasNext())
    {
        it.next();
        
        if(it.value() != 0 && !inInterval)
        {
            inInterval = true;
            curInterval.first = it.key();
            lastKeyInInter = it.key();
            // areaCur = it.value(); // modif PhD on 2015/05/20
            areaCur = 0.; // start, not inside
        }
        else if(inInterval)
        {
            if((it.value() == 0) )
            {
                inInterval = false;
                curInterval.second = lastKeyInInter;
                
                QPair<double, QPair<double, double> > inter;
                inter.first = thresh * areaCur / areaTot;
                inter.second = curInterval;
                intervals.append(inter);
                
                areaCur = 0;
                //qDebug()<<" inInerval second"<<curInterval.first<<curInterval.second;
            }
            else
            {
                
                //areaCur += it.value(); // modif PhD on 2015/05/20
                
                areaCur += (lastValueInInter+it.value())/2 * (it.key()-lastKeyInInter);
             
                lastKeyInInter = it.key();
                lastValueInInter = it.value();
                
            }
        }
    }
    
    if (inInterval) { // Modif PhD correction to close unclosed interval
       
        curInterval.second = lastKeyInInter;
        QPair<double, QPair<double, double> > inter;
        inter.first = thresh * areaCur / areaTot;
        inter.second = curInterval;
        //qDebug()<<"second"<<curInterval.first<<curInterval.second;
        intervals.append(inter);
    }
    
    return intervals;
}