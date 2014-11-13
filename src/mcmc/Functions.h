#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <QMap>
#include <QVector>


struct FunctionAnalysis{
    float max = 0.;
    float mode = 0.;
    float mean = 0.;
    float variance = 0.;
};

FunctionAnalysis analyseFunction(const QMap<float, float>& aFunction);

// Standard Deviation (= écart type) of a vector of data
float dataStd(const QVector<float>& data);

#endif