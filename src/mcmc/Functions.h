#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <QMap>
#include <QVector>


struct FunctionAnalysis{
    double max = 0.;
    double mode = 0.;
    double mean = 0.;
    double variance = 0.;
};

FunctionAnalysis analyseFunction(const QMap<float, float>& aFunction);

// Standard Deviation (= écart type) of a vector of data
float dataStd(const QVector<float>& data);

#endif