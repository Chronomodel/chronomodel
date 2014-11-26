#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <QMap>
#include <QVector>


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

#endif