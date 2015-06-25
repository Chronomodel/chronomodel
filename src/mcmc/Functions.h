#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <QMap>
#include <QVector>
#include <cmath>
#include "StdUtilities.h"


struct FunctionAnalysis{
    double max = 0.f;
    double mode = 0.f;
    double mean = 0.f;
    double stddev = 0.f;
};

struct Quartiles{
    double Q1 = 0.f;
    double Q2 = 0.f;
    double Q3 = 0.f;
};

struct DensityAnalysis
{
    Quartiles quartiles;
    FunctionAnalysis analysis;
};

FunctionAnalysis analyseFunction(const QMap<double, double>& aFunction);
QString functionAnalysisToString(const FunctionAnalysis& analysis);
QString densityAnalysisToString(const DensityAnalysis& analysis);

// Standard Deviation (= écart type) of a vector of data
double dataStd(const QVector<double>& data);

double shrinkageUniform(double so2);

Quartiles quartilesForTrace(const QVector<double>& trace);
Quartiles quartilesForRepartition(const QVector<double>& repartition, double tmin, double step);
QPair<double, double> credibilityForTrace(const QVector<double>& trace, double thresh, double& exactThresholdResult);
QString intervalText(const QPair<double, QPair<double, double> >& interval, FormatFunc formatFunc = 0);
QString getHPDText(const QMap<double, double>& hpd, double thresh, const QString& unit = QString(), FormatFunc formatFunc = 0);
QList<QPair<double, QPair<double, double> > > intervalsForHpd(const QMap<double, double>& hpd, double thresh);

inline double rounddouble(double f, int prec)
{
    double result;
    if(prec > 0)
    {
        double factor = pow(10.f, (double)prec);
        result = round(f * factor) / factor;
    }
    else
    {
        result = round(f);
    }
    return result;
}

#endif