#include "Functions.h"
#include "Generator.h"
#include "StdUtilities.h"
#include "DateUtils.h"
#include <QDebug>
#include <QProgressDialog>
#include <QApplication>
#include <set>
#include <map>
#include <QTime>

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
    QString result (QObject::tr("No data"));
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
    const int n = trace.size();
    if (n<5) {
        quartiles.Q1 = 0.;
        quartiles.Q2 = 0.;
        quartiles.Q3 = 0.;
        return quartiles;
    }
    QVector<double> sorted (trace);
    std::sort(sorted.begin(),sorted.end());
    
    const int q1index = ceil((double)n * 0.25f);
    const int q3index = ceil((double)n * 0.75f);
    
    quartiles.Q1 = sorted.at(q1index);
    quartiles.Q3 = sorted.at(q3index);
    
    if (n % 2 == 0) {
        const int q2indexLow = n / 2;
        const int q2indexUp = q2indexLow + 1;
        
        quartiles.Q2 = sorted.at(q2indexLow) + (sorted.at(q2indexUp) - sorted.at(q2indexLow)) / 2.f;
    } else {
        const int q2index = ceil((double)n * 0.5f);
        quartiles.Q2 = sorted.at(q2index);
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
    QPair<double, double> credibility(0.,0.);
    //credibility.first = 0.;
    //credibility.second = 0.;
    exactThresholdResult = 0.;
    const int n = trace.size();
    if (thresh > 0 && n > 0) {
        /*double threshold =  (thresh > 100 ? thresh = 100.0 : thresh);
        threshold = (thresh < 0 ? thresh = 0.0 : thresh);*/
        double threshold = inRange(0.0, thresh, 100.0);
        QVector<double> sorted (trace);
        std::sort(sorted.begin(),sorted.end());
        
        const int numToRemove = (int)floor((double)n * (1.f - threshold / 100.f));
        exactThresholdResult = ((double)n - (double)numToRemove) / (double)n;
        
       // const int k = numToRemove;
        //const int n = sorted.size();
        double lmin = 0.f;
        int foundJ = 0;

        for (int j=0; j<=numToRemove; ++j) {
            const double l = sorted.at((n - 1) - numToRemove + j) - sorted.at(j);
            if ((lmin == 0.f) || (l < lmin)) {
                foundJ = j;
                lmin = l;
            }
        }
        credibility.first = sorted.at(foundJ);
        credibility.second = sorted.at((n - 1) - numToRemove + foundJ);
    }

    if (credibility.first == credibility.second) {
        //It means : there is only on value
        return QPair<double, double>();
    }
    else return credibility;
}

QPair<double, double> timeRangeFromTraces(const QVector<double>& trace1, const QVector<double>& trace2, const double thresh, const QString description)
{
    QPair<double, double> range(- INFINITY, +INFINITY);
#ifdef DEBUG
    QTime startTime (QTime::currentTime());
#endif
    // limit of precision, to accelerate the calculus
    const float perCentStep = 0.01f;
    // if thresh is equal 0 then return an QPair=(-INFINITY,+INFINITY)

    const int n = trace1.size();
    if ( (thresh > 0) && (n > 0) && (trace2.size() == n) ) {

        const int nTarget = (const int)(ceil((double)n * thresh/100.));
        const int nGamma = n - nTarget;

        double dMin = INFINITY;

        std::vector<double> traceAlpha (trace1.toStdVector());
        std::vector<double> traceBeta (trace2.size());

        // map with relation Beta to Alpha
        std::multimap<double,double> betaAlpha;
        for(int i=0; i<trace1.size(); ++i)
            betaAlpha.insert(std::pair<double,double>(trace2.at(i),trace1.at(i)) );//std::pair<char,int>('a',100)

        // std::vector<double> traceBeta2 = trace2.toStdVector();
        std::copy(trace2.begin(),trace2.end(),traceBeta.begin());


        // keep the beta trace in the same position of the Alpha, so we need to sort them with there values of alpha
        std::sort(traceBeta.begin(),traceBeta.end(),[&betaAlpha](const double i, const double j){ return betaAlpha.find(i)->second <betaAlpha.find(j)->second  ;} );

        std::sort(traceAlpha.begin(),traceAlpha.end());

        if (nTarget>= n)
            return QPair<double,double>(traceAlpha.at(0),*std::max_element(traceBeta.cbegin(),traceBeta.cend()));

        // Display a progress bar
      //  QProgressDialog *progress = new QProgressDialog(description,"Wait" , 1, 10, qApp->activeWindow() );
      //  progress->setWindowModality(Qt::WindowModal);
      //  progress->setCancelButton(0);
        //progress->forceShow();
      //  progress->setMinimumDuration(4);

     //   progress->setMinimum(0);
     //   progress->setMaximum(nGamma);
        const int epsilonStep = qMax(1, (int)floor(n*perCentStep));

        std::vector<double> betaUpper(n);

        for (int nEpsilon=0; nEpsilon<nGamma; ) {
     //       progress->setValue(nEpsilon);

            const double a = traceAlpha.at(nEpsilon);

            // copy only value of beta with alpha greater than a(epsilon)

            const int remainingElemt =  n - nEpsilon;
            betaUpper.resize(remainingElemt);   // allocate space

            auto it = std::copy( traceBeta.begin()+ nEpsilon+1, traceBeta.end(), betaUpper.begin() );

            // copy only positive numbers:
            // auto it = std::copy_if(traceBeta.begin()+ nEpsilon+1, traceBeta.end(), betaUpper.begin(), [&a](double i){ return !(i<a);} );
             const int betaUpperSize = std::distance(betaUpper.begin(),it);

             betaUpper.resize(betaUpperSize);  // shrink container to new size

             // if there is Beta value under a, we could have less than nTarget-1 elements, so it's finish
             if (betaUpperSize<(nTarget-1))
                 break;

            /*  std::nth_element has O(N) complexity,
             *  whereas std::sort has O(Nlog(N)).
             *  here we don't need complete sorting of the range, so it's advantageous to use it.
             */

            std::nth_element(betaUpper.begin(), betaUpper.begin() + nTarget-1, betaUpper.end());
//std::sort(betaUpper.begin(),  betaUpper.end());
// in the future with C++17
//std::experimental::parallel::nth_element(par,betaUpper.begin(), betaUpper.begin() + nTarget, betaUpper.end());

            // Remember ->  m*(n-nGamma)/(n-nEpsilon) = nTarget
            const double b  = betaUpper.at(nTarget-1);

            // keep the shortest length
            if ((b-a) < dMin) {
                dMin = b - a;
                range.first = a;
                range.second = b;

                // compute type 7 R quantile
              /*
                const float ha = n*(thresh/100);
                const int floorHa = qMin((int)nEpsilon,(int)(traceAlpha.size()-1-epsilonStep));//(int)floor(ha);

                const double a7 = traceAlpha.at(floorHa)+((ha-floor(ha))*(traceAlpha.at(floorHa+epsilonStep)-traceAlpha.at(floorHa)));

               const float hb = n*(thresh/100);//(n-1)*(nEpsilon/n)+1;
               const int floorHb = qMin(nTarget-1,(int)(betaUpper.size()-1-epsilonStep));//(int)floor(hb);

               const double b7 = betaUpper.at(floorHb)+((hb-floor(hb))*(betaUpper.at(floorHb+epsilonStep)-betaUpper.at(floorHb)));
               range.first = a7;
               range.second = b7;*/
            }

            nEpsilon += epsilonStep;
        }
//        delete progress;
    }


#ifdef DEBUG
    qDebug()<<description;
    QTime timeDiff(0,0,0,1);
    timeDiff = timeDiff.addMSecs(startTime.elapsed()).addMSecs(-1);
    qDebug()<<"timeRangeFromTraces ->time elapsed = "<<timeDiff.hour()<<"h "<<QString::number(timeDiff.minute())<<"m "<<QString::number(timeDiff.second())<<"s "<<QString::number(timeDiff.msec())<<"ms" ;
#endif
    return range;
}

QPair<double, double> transitionRangeFromTraces(const QVector<double>& trace1, const QVector<double>& trace2, const double thresh, const QString description)
{
    return timeRangeFromTraces(trace1, trace2, thresh, description);
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
    QPair<double, double> range = QPair<double, double>(- INFINITY, + INFINITY);
#ifdef DEBUG
    QTime startTime = QTime::currentTime();
#endif
    // limit of precision, to accelerate the calculus
    const float perCentStep = 0.01f;

    const int n = traceBeta.size();
    if ( (thresh > 0) && (n > 0) && (traceAlpha.size() == n) ) {

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

        std::vector<double> alphaUnder;
        alphaUnder.reserve(n);
        std::multimap<double,double>::const_reverse_iterator iMapTmp = i_shift;


        const int epsilonStep = qMax(1, (int)floor(nGamma*perCentStep));

        for (int nEpsilon = 0; (nEpsilon <= nGamma ) && (i_shift != mapPair.rend()); ) {

            // We use a reverse Iterator so the first is the last value in the QMap
            const double a = (*i_shift).first;//a=beta(i)=a(epsilon);

            // find position of beta egual a(epsilon)
            iMapTmp = i_shift;
            alphaUnder.clear();
            while (iMapTmp != mapPair.rend()) {
                alphaUnder.push_back((*iMapTmp).second);
                ++iMapTmp;
            }
            //std::sort(alphaUnder.begin(),alphaUnder.end());

            const int j = nGamma - nEpsilon;
            std::nth_element(alphaUnder.begin(), alphaUnder.begin() + j, alphaUnder.end());

            const double b = alphaUnder.at(j); //b=alpha(j)
            // keep the longest length
            if ((b-a) > dMax) {
                dMax = b-a;
                range.first = a;
                range.second = b;
            }

            nEpsilon = nEpsilon + epsilonStep;
            if (nEpsilon<=nGamma)
                std::advance(i_shift,epsilonStep);// reverse_iterator

        }

    }

#ifdef DEBUG
    qDebug()<<description;
    QTime timeDiff(0,0,0,1);
    timeDiff = timeDiff.addMSecs(startTime.elapsed()).addMSecs(-1);
    qDebug()<<"gapRangeFromTraces ->time elapsed = "<<timeDiff.hour()<<"h "<<QString::number(timeDiff.minute())<<"m "<<QString::number(timeDiff.second())<<"s "<<QString::number(timeDiff.msec())<<"ms" ;
#endif
    return range;
}



QString intervalText(const QPair<double, QPair<double, double> >& interval, FormatFunc formatFunc)
{
    const QLocale locale;
    if (formatFunc){
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
    for(int i=0; i<intervals.size(); ++i) {
        results << intervalText(intervals.at(i), formatFunc);
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
    while(it.hasNext()) {
        it.next();
        
        if(it.value() != 0 && !inInterval) {
            inInterval = true;
            curInterval.first = it.key();
            lastKeyInInter = it.key();
            areaCur = 0.; // start, not inside
        } else if(inInterval) {
            if((it.value() == 0) ) {
                inInterval = false;
                curInterval.second = lastKeyInInter;
                
                QPair<double, QPair<double, double> > inter;
                inter.first = thresh * areaCur / areaTot;
                inter.second = curInterval;
                intervals.append(inter);
                
                areaCur = 0;

            } else {
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
