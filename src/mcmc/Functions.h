#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <QMap>
#include <QVector>
#include <cmath>


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

FunctionAnalysis analyseFunction(const QMap<float, float>& aFunction);
QString functionAnalysisToString(const FunctionAnalysis& analysis);
QString densityAnalysisToString(const DensityAnalysis& analysis);

// Standard Deviation (= écart type) of a vector of data
float dataStd(const QVector<float>& data);

float shrinkageUniform(float so2);

Quartiles quartilesForTrace(const QVector<float>& trace);
Quartiles quartilesForRepartition(const QVector<float>& repartition, float tmin, float step);
QPair<float, float> credibilityForTrace(const QVector<float>& trace, int threshold, float& exactThresholdResult);
QString intervalText(const QPair<float, float>& interval);
QString getHPDText(const QMap<float, float>& hpd);
QList<QPair<float, float>> intervalsForHpd(const QMap<float, float>& hpd);

inline float roundFloat(float f, int prec)
{
    float result;
    if(prec > 0)
    {
        float factor = powf(10.f, (float)prec);
        result = roundf(f * factor) / factor;
    }
    else
    {
        result = roundf(f);
    }
    return result;
}

#endif