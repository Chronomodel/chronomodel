#include "Functions.h"
#include "Generator.h"
#include "StdUtilities.h"
#include "DateUtils.h"
#include <QDebug>
#include <QProgressDialog>
#include <QApplication>
#include <set>
#include <map>

// -----------------------------------------------------------------
//  sumP = Sum (pi)
//  sum = Sum (pi * xi)
//  sum2 = Sum (pi * xi^2)
// -----------------------------------------------------------------

/**
 * @brief Product a FunctionAnalysis from a QMap
 * @todo Handle empty function case and null density case (pi = 0)
*/
FunctionAnalysis analyseFunction(const QMap<double, double>& aFunction)
{
    FunctionAnalysis result;
    if (aFunction.isEmpty()) {
        result.max = 0;
        result.mode = 0;
        result.mean = 0;
        result.stddev = -1;
        qDebug() << "WARNING : in analyseFunction() aFunction isEmpty !! ";
        return result;
    }
    
    double max = 0.;
    double mode = 0.;
    double sum = 0.;
    double sum2 = 0.;
    double sumP = 0.;
    
    double prevY = 0;
    QList<double> uniformXValues;
    
    QMap<double,double>::const_iterator citer = aFunction.cbegin();
    for (;citer != aFunction.cend(); ++citer) {
        const double x = citer.key();
        const double y = citer.value();
        
        sumP += y;
        sum += y * x;
        sum2 += y * x * x;
        
        if (max <= y) {
            max = y;
            if (prevY == y) {
                uniformXValues.append(x);
                int middleIndex = floor(uniformXValues.size()/2);
                mode = uniformXValues.at(middleIndex);
            } else {
                uniformXValues.clear();
                mode = x;
            }
        }
        prevY = y;
    }
    
    //FunctionAnalysis result;
    result.max = max;
    result.mode = mode;
    result.mean = 0;
    result.stddev = 0;
    
    if (sumP != 0) {
        result.mean = sum / sumP;
        double variance = (sum2 / sumP) - pow(result.mean, 2);
        
        if (variance < 0) {
            qDebug() << "WARNING : in analyseFunction() negative variance found : " << variance<<" return 0";
            variance = -variance;
        }
        
        result.stddev = sqrt(variance);
    }
    
    return result;
}

double dataStd(const QVector<double>& data)
{
    // Work with double precision here because sum2 might be big !
    
    const double s = sum<double>(data);
    const double s2 = sum2<double>(data);
    const double mean = s / data.size();
    const double variance = s2 / data.size() - mean * mean;
    
    if (variance < 0) {
        qDebug() << "WARNING : in dataStd() negative variance found : " << variance<<" return 0";
        return 0.f;
    }
    return (double)sqrt(variance);
}

double shrinkageUniform(const double so2)
{
    double u = Generator::randomUniform();
    return (so2 * (1. - u) / u);
}

/**
 * @brief Return a text from a FunctionAnalysis
 * @see FunctionAnalysis
 */
QString functionAnalysisToString(const FunctionAnalysis& analysis)
{
    QString result;

    if (analysis.stddev<0.)
        result = QObject::tr("No data");
    else {
        result += "MAP : " + DateUtils::dateToString(analysis.mode) + "   ";
        result += "Mean : " + DateUtils::dateToString(analysis.mean) + "   ";
        result += "Std deviation : " +DateUtils::dateToString(analysis.stddev);
    }
    return result;
}

/**
 * @brief Return a text with the value of th Quartiles Q1, Q2 and Q3
 * @see DensityAnalysis
 */
QString densityAnalysisToString(const DensityAnalysis& analysis, const QString& nl)
{
    QString result = QObject::tr("No data");
    if (analysis.analysis.stddev>=0.) {
        result = functionAnalysisToString(analysis.analysis) + nl;
        result += "Q1 : " + DateUtils::dateToString(analysis.quartiles.Q1) + "   ";
        result += "Q2 (Median) : " + DateUtils::dateToString(analysis.quartiles.Q2) + "   ";
        result += "Q3 : " + DateUtils::dateToString(analysis.quartiles.Q3);
    }
    return result;
}


Quartiles quartilesForTrace(const QVector<double>& trace)
{
    Quartiles quartiles;
    if (trace.size()<5) {
        quartiles.Q1 = 0.;
        quartiles.Q2 = 0.;
        quartiles.Q3 = 0.;
        return quartiles;
    }
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

Quartiles quartilesForRepartition(const QVector<double>& repartition, const double tmin, const double step)
{
    Quartiles quartiles;
    if (repartition.size()<5) {
        quartiles.Q1 = 0.;
        quartiles.Q2 = 0.;
        quartiles.Q3 = 0.;
        return quartiles;
    }
    const double q1index = vector_interpolate_idx_for_value(0.25, repartition);
    const double q2index = vector_interpolate_idx_for_value(0.5, repartition);
    const double q3index = vector_interpolate_idx_for_value(0.75, repartition);
    
    quartiles.Q1 = tmin + q1index * step;
    quartiles.Q2 = tmin + q2index * step;
    quartiles.Q3 = tmin + q3index * step;
    
    return quartiles;
}

QPair<double, double> credibilityForTrace(const QVector<double>& trace, double thresh, double& exactThresholdResult,const  QString description)
{
    QPair<double, double> credibility;
    credibility.first = 0.;
    credibility.second = 0.;
    exactThresholdResult = 0.;
    
    // Diplay a progressBar if long
    QProgressDialog *progress = new QProgressDialog(description,"Wait" , 1, 10,qApp->activeWindow());
    progress->setWindowModality(Qt::WindowModal);
    progress->setCancelButton(0);
    //progress->forceShow();
    //progress->setMinimumDuration(40);

    if (thresh > 0 && trace.size() > 0) {
        /*double threshold =  (thresh > 100 ? thresh = 100.0 : thresh);
        threshold = (thresh < 0 ? thresh = 0.0 : thresh);*/
        double threshold = inRange(0.0, thresh, 100.0);
        QVector<double> sorted = trace;
        qSort(sorted);
        
        int numToRemove = floor((double)sorted.size() * (1.f - threshold / 100.f));
        exactThresholdResult = ((double)sorted.size() - (double)numToRemove) / (double)sorted.size();
        
        const int k = numToRemove;
        const int n = sorted.size();
        double lmin = 0.f;
        int foundJ = 0;
        progress->setMaximum(k);

        for (int j=0; j<=k; ++j) {
            progress->setValue(k);
            const double l = sorted[(n - 1) - k + j] - sorted[j];
            if (lmin == 0.f || l < lmin) {
                foundJ = j;
                lmin = l;
            }
        }
        credibility.first = sorted[foundJ];
        credibility.second = sorted[(n - 1) - k + foundJ];
    }

    delete progress;

    if (credibility.first == credibility.second) {
        //It means : there is only on value
        return QPair<double, double>();
    }
    else return credibility;
}


QPair<double, double> timeRangeFromTraces(const QVector<double>& trace1, const QVector<double>& trace2, const double thresh, const QString description)
{
    QPair<double, double> range;
    range.first = - INFINITY;
    range.second = + INFINITY;
    
   /* if(thresh == 100) {
        range.first = *(std::min_element(trace1.cbegin(),trace1.cend()));
        range.second = *(std::max_element(trace2.cbegin(),trace2.cend()) );
    }*/
    
    // Display a progress bar
    QProgressDialog *progress = new QProgressDialog(description,"Wait" , 1, 10, qApp->activeWindow() );
    progress->setWindowModality(Qt::WindowModal);
    progress->setCancelButton(0);
    //progress->forceShow();
    progress->setMinimumDuration(4);
    
    // if thresh is equal 0 then return an QPair=(-INFINITY,+INFINITY)
    
    const int n = trace1.size();
    if ( (thresh > 0) && (n > 0) && (trace2.size() == n) ) {
        
        const int nTarget = (const int)(ceil((double)n * thresh/100.));
        const int nGamma = n - nTarget;
        
        double dMin = INFINITY;
        
        // make couple in a std::map
        std::multimap<double,double> mapPair;
        QVector<double>::const_iterator ct1 = trace1.cbegin();
        QVector<double>::const_iterator ct2 = trace2.cbegin();


        while (ct1 != trace1.cend() ) {
            mapPair.insert(std::pair<double,double>(*ct1,*ct2));
            ++ct1;
            ++ct2;
        }
        
        // we suppose there is never the several time the same value inside trace1 or inside trace2
        // so we can juste shift the iterator
        std::multimap<double,double>::const_iterator i_shift = mapPair.cbegin();
        
        progress->setMinimum(0);
        progress->setMaximum(nGamma);
        for (int nEpsilon=0; (nEpsilon<=nGamma) && (i_shift != mapPair.cend()); ++nEpsilon) {
            progress->setValue(nEpsilon);
            
            // memory alpha.at(nEpsilon);
            const double a = (*i_shift).first;

            
            // find all value of alpha greater than a
            std::multimap<double,double>::const_iterator iMap = mapPair.find(a);
            
            // copy only value of beta greater than a(epsilon)
            std::multiset<double> betaUpper;
            while (iMap != mapPair.cend()) {
                 betaUpper.insert((*iMap).second);
                 ++iMap;
             }

           // Remember ->  m*(n-nGamma)/(n-nEpsilon) = nTarget
            
          
            std::multiset<double>::const_iterator j_shift = betaUpper.cbegin();
            // I'm already on the first one
            
            // loop to move the iterator to the value number nTarget, the SET have not a Random-acces iterator
            for (int i=1; (i<nTarget) && (j_shift != betaUpper.cend()); ++i)
                ++j_shift;

            const double b  = *j_shift;
       

            // keep the shortest length
            if ((b-a) < dMin) {
                dMin = b - a;
                range.first = a;
                range.second = b;
            }
        ++i_shift;
        }

    }
    delete progress;
    return range;
}

/**
 * @brief gapRangeFromTraces find the gap between two traces, if there is no solution corresponding to the threshold, we return a QPair=(-INFINITY,+INFINITY)
 * @param traceBeta QVector of double corresponding to the first trace
 * @param traceAlpha QVector of double corresponding to the second trace
 * @param thresh Threshold to obtain
 * @param description a simple text
 * @return
 */
QPair<double, double> gapRangeFromTraces(const QVector<double>& traceBeta, const QVector<double>& traceAlpha, const double thresh, const QString description)
{
    QPair<double, double> range;

    range.first = - INFINITY;
    range.second = + INFINITY;

    QProgressDialog *progress = new QProgressDialog(description,"Wait" , 1, 10, qApp->activeWindow() );
    progress->setWindowModality(Qt::WindowModal);
    progress->setCancelButton(0);
    //progress->forceShow();
    progress->setMinimumDuration(4);

    const int n = traceBeta.size();
    if ( (thresh > 0) && (n > 0) && (traceAlpha.size() == n) ) {
        //const double threshold =  inRange(0., thresh, 100.0);

        const int nTarget = (const int) ceil( (double)n * thresh/100.);
        const int nGamma = n - nTarget;

        double dMax = 0.0;

        // make couple beta vs alpha in a std::map, it's a sorted container with ">"
        std::multimap<double,double> mapPair;
        QVector<double>::const_iterator ctB = traceBeta.cbegin();
        QVector<double>::const_iterator ctA = traceAlpha.cbegin();

        while (ctB != traceBeta.cend() ) {
            mapPair.insert(std::pair<double,double>(*ctB,*ctA));
            ++ctA;
            ++ctB;
        }

        // we suppose there is never the several time the same value inside traceBeta or inside traceAlpha
        // so we can just shift the iterator
        std::multimap<double,double>::const_reverse_iterator i_shift = mapPair.rbegin();

        progress->setMaximum(nGamma);
        progress->setMinimum(0);

        for (int nEpsilon = 0; (nEpsilon <= nGamma ) && (i_shift != mapPair.rend()); ++nEpsilon) {
            progress->setValue(nEpsilon);
            
            
            //const int m = n - nEpsilon;
            
            // if j<0, target is not reachable so there is no solution or the loop is finish!
           /* if (j < 0) {
                delete progress;
                return range;
            }*/
            
            // We use a reverse Iterator so the first is the last value in the QMap
            const double a = (*i_shift).first;//a=beta(i)=a(epsilon);

            // find position of beta egual a(epsilon)
            std::multiset<double> alphaUnder; // sorted container
            std::multimap<double,double>::const_iterator iMap = mapPair.find(a);
            alphaUnder.clear();
            while (iMap != mapPair.cbegin()) {
                alphaUnder.insert((*iMap).second);
                --iMap;
            }
            // insert the value corresponding to cbegin
            alphaUnder.insert((*iMap).second);

      
            std::multiset<double>::const_iterator j_shift = alphaUnder.cbegin();

            // loop to move the iterator to the value number j, the SET have NO Random-acces iterator
            // I'm already on the first element
            const int j = nGamma - nEpsilon;
            for (int i = 1; (i <= j) && (j_shift != alphaUnder.cend()); ++i)
                ++j_shift;

            const double b = *j_shift; //b=alpha(j)

            // keep the longest length
            if ((b-a) > dMax) {
                dMax = b-a;
                range.first = a;
                range.second = b;
            }
            ++i_shift; // reverse_iterator
        }

    }
    delete progress;
    return range;
}


QString intervalText(const QPair<double, QPair<double, double> >& interval, FormatFunc formatFunc)
{
    const QLocale locale;
    if(formatFunc){
        return "[" + formatFunc(interval.second.first) + "; " + formatFunc(interval.second.second) + "] (" + locale.toString(interval.first, 'f', 1) + "%)";
    }
    else {
        return "[" + DateUtils::dateToString(interval.second.first) + "; " + DateUtils::dateToString(interval.second.second) + "] (" + locale.toString(interval.first, 'f', 1) + "%)";
    }
}

QString getHPDText(const QMap<double, double>& hpd, double thresh, const QString& unit, FormatFunc formatFunc)
{
    if(hpd.isEmpty() ) return "";

    QList<QPair<double, QPair<double, double> > > intervals = intervalsForHpd(hpd, thresh);
    QStringList results;
    for(int i=0; i<intervals.size(); ++i)
    {
        results << intervalText(intervals[i], formatFunc);
    }
    QString result = results.join(", ");
    if(!unit.isEmpty()) {
        result += " " + unit;
    }
    return result;
}
/**
 * @brief Extract intervals (QPair of date) and calcul the area corresponding, from a HPD QMap maded before
 */
QList<QPair<double, QPair<double, double> > > intervalsForHpd(const QMap<double, double>& hpd, double thresh)
{
    QList<QPair<double, QPair<double, double> >> intervals;

    if(hpd.isEmpty()) return intervals;

    QMapIterator<double, double> it(hpd);
    bool inInterval = false;
    double lastKeyInInter = 0.;
    QPair<double, double> curInterval;
    
    double areaTot= map_area(hpd);
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

            }
            else
            {
                areaCur += (lastValueInInter+it.value())/2 * (it.key()-lastKeyInInter);
             
                lastKeyInInter = it.key();
                lastValueInInter = it.value();
                
            }
        }
    }
    
    if (inInterval) { // Correction to close unclosed interval
       
        curInterval.second = lastKeyInInter;
        areaCur += (lastValueInInter+it.value())/2 * (it.key()-lastKeyInInter);
        QPair<double, QPair<double, double> > inter;
        inter.first = thresh * areaCur / areaTot;
        inter.second = curInterval;

        intervals.append(inter);
    }
    
    return intervals;
}
