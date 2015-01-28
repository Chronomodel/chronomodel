#ifndef GENERATOR_H
#define GENERATOR_H

#include "random"

#ifndef M_PI
#define M_PI 3.1415927
#endif

class Generator
{
public:
    static int createSeed();
    static void initGenerator(const int seed);
    
    static double randomUniform(double min = 0., double max = 1.);
    static double gaussByDoubleExp(const double mean, const double sigma, const double min, const double max);
    static double gaussByBoxMuller(const double mean, const double sigma);
    
private:
    Generator(){}
    static double boxMuller();
    
    static std::mt19937 sGenerator;
    static std::uniform_real_distribution<double> sDistribution;
};

#endif
