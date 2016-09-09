#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <QMap>
#include <QVector>
#include <cmath>
#include "StdUtilities.h"


struct FunctionAnalysis{
    float max = 0.f;
    float mode = 0.f;
    float mean = 0.f;
    float stddev = 0.f;
};

struct Quartiles{
    float Q1 = 0.f;
    float Q2 = 0.f;
    float Q3 = 0.f;
};

struct DensityAnalysis
{
    Quartiles quartiles;
    FunctionAnalysis analysis;
};

FunctionAnalysis analyseFunction(const QMap<double, double>& aFunction);
FunctionAnalysis analyseFunction(const QMap<float, float>& aFunction);

QString functionAnalysisToString(const FunctionAnalysis& analysis);
QString densityAnalysisToString(const DensityAnalysis& analysis, const QString& nl = "<br>");

// Standard Deviation of a vector of data
float dataStd(const QVector<float> &data);

double shrinkageUniform(const double so2);

Quartiles quartilesForTrace(const QVector<float>& trace);
Quartiles quartilesForRepartition(const QVector<float>& repartition,const float tmin,const float step);
QPair<float, float> credibilityForTrace(const QVector<float>& trace, float thresh, float& exactThresholdResult, const QString description = "Credibility computation");
QPair<float, float> timeRangeFromTraces(const QVector<float>& trace1, const QVector<float>& trace2, const float thresh, const QString description ="Time Range Computation");

QPair<float, float> gapRangeFromTraces(const QVector<float>& trace1, const QVector<float>& trace2, const float thresh, const QString description ="Gap Range Computation");
QPair<float, float> transitionRangeFromTraces(const QVector<float>& trace1, const QVector<float>& trace2, const float thresh, const QString description ="Gap Range Computation");

QString intervalText(const QPair<float, QPair<float, float> >& interval, FormatFunc formatFunc = 0);
QString getHPDText(const QMap<float, float>& hpd, float thresh, const QString& unit = QString(), FormatFunc formatFunc = 0);
QList<QPair<float, QPair<float, float> > > intervalsForHpd(const QMap<float, float>& hpd, float thresh);

inline double rounddouble(const double f,const int prec)
{
    double result;
    if (prec > 0){
        const double factor = pow(10.f, (double)prec);
        result = round(f * factor) / factor;
    } else {
        result = round(f);
    }
    return result;
}

#endif
