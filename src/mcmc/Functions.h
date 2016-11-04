#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <QMap>
#include <QVector>
#include <cmath>
#include "StdUtilities.h"
typedef double type_data;

struct FunctionAnalysis{
    type_data max = (type_data)0.;
    type_data mode = (type_data)0.;
    type_data mean = (type_data)0.;
    type_data stddev = (type_data)0.;
};

struct Quartiles{
    type_data Q1 = (type_data)0.;
    type_data Q2 = (type_data)0.;
    type_data Q3 = (type_data)0.;
};

struct DensityAnalysis
{
    Quartiles quartiles;
    FunctionAnalysis analysis;
};

FunctionAnalysis analyseFunction(const QMap<type_data, type_data>& aFunction);

QString functionAnalysisToString(const FunctionAnalysis& analysis);
QString densityAnalysisToString(const DensityAnalysis& analysis, const QString& nl = "<br>");

// Standard Deviation of a vector of data
type_data dataStd(const QVector<type_data> &data);

double shrinkageUniform(const double so2);

Quartiles quartilesForTrace(const QVector<type_data>& trace);
Quartiles quartilesForRepartition(const QVector<double> &repartition, const double tmin, const double step);
QPair<double, double> credibilityForTrace(const QVector<double>& trace, double thresh, double& exactThresholdResult, const QString description = "Credibility computation");
QPair<double, double> timeRangeFromTraces(const QVector<double>& trace1, const QVector<double>& trace2, const double thresh, const QString description ="Time Range Computation");


QPair<double, double> gapRangeFromTraces(const QVector<double>& trace1, const QVector<double>& trace2, const double thresh, const QString description ="Gap Range Computation");

QPair<double, double> transitionRangeFromTraces(const QVector<double> &trace1, const QVector<double> &trace2, const double thresh, const QString description ="Gap Range Computation");

QString intervalText(const QPair<double, QPair<double, double> >& interval, FormatFunc formatFunc = 0);
QString getHPDText(const QMap<double, double>& hpd, double thresh, const QString& unit = QString(), FormatFunc formatFunc = 0);
QList<QPair<double, QPair<double, double> > > intervalsForHpd(const QMap<double, double> &hpd, double thresh);

inline double rounddouble(const double f,const int prec)
{
    double result;
    if (prec > 0){
        const double factor = pow(10., (double)prec);
        result = round(f * factor) / factor;
    } else {
        result = round(f);
    }
    return result;
}

#endif
